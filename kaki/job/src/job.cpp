//
// Created by felix on 13/04/2022.
//

#include "job.h"

#include <boost/context/fiber.hpp>
#include <boost/context/continuation.hpp>
#include <boost/pool/pool.hpp>
#include <boost/lockfree/queue.hpp>

#include <queue>

#include <condition_variable>
#include <mutex>
#include <atomic>

namespace kaki {

    struct JobWait {
        boost::context::fiber fiber;
        std::atomic<uint64_t>* waitOn;
        uint64_t value;
    };

    struct Scheduler::Impl {
        std::mutex jobQueueMutex;
        std::condition_variable jobQueueCondVar;
        std::queue<JobFn> jobQueue;

        std::vector<JobWait> waitJobs;

        std::atomic<bool> shutdown;

        JobFn popNextJob();
        boost::context::fiber nextFiber();
    };

    void Scheduler::scheduleJob(JobFn &&job) {
        std::unique_lock lock(impl->jobQueueMutex);
        impl->jobQueue.emplace(std::forward<JobFn>(job));
        lock.unlock();
        impl->jobQueueCondVar.notify_one();
    }

    Scheduler::Scheduler() {
        impl = new Impl();
    }

    Scheduler::~Scheduler() {
        delete impl;
    }

    struct JobCtxI
    {
        boost::context::fiber fiber;
    };

    JobFn Scheduler::Impl::popNextJob() {
        std::unique_lock lock(jobQueueMutex);
        jobQueueCondVar.wait(lock, [this]() {
            return !jobQueue.empty();
        });

        JobFn job = std::move(jobQueue.front());
        jobQueue.pop();
        lock.unlock();

        return std::move(job);
    }

    static thread_local std::atomic<uint64_t>* s_tl_waitOnPtr = nullptr;
    static thread_local uint64_t s_tl_waitOnValue = 0;

    boost::context::fiber Scheduler::Impl::nextFiber() {
        for(size_t i = 0; i < waitJobs.size(); i++)
        {
            if (waitJobs[i].waitOn->load() == waitJobs[i].value) {
                boost::context::fiber fiber = std::move(waitJobs[i].fiber);
                waitJobs.erase(waitJobs.begin() + i);
                return fiber;
            }
        }

        // no ready waiting jobs, get next job in queue
        JobFn job = popNextJob();
        boost::context::fiber fiber([job = std::move(job)](boost::context::fiber ctx) {
            JobCtxI ctxi {
                    .fiber = std::move(ctx),
            };
            job(JobCtx {
                    .handle = &ctxi,
            });
            return std::move(ctxi.fiber);
        });

        return fiber;
    }

    void Scheduler::run() {
        while(!impl->shutdown) {

            auto fiber = impl->nextFiber();

            fiber = std::move(fiber).resume();

            if ( s_tl_waitOnPtr )
            {
                impl->waitJobs.push_back( JobWait{
                    .fiber = std::move(fiber),
                    .waitOn = s_tl_waitOnPtr,
                    .value = s_tl_waitOnValue,
                });
            }
            s_tl_waitOnPtr = nullptr;
        }
    }

    void Scheduler::shutdownNow() {
        impl->shutdown = true;
    }

    void JobCtx::wait(std::atomic<uint64_t> &atomic, uint64_t value) {
        s_tl_waitOnPtr = &atomic;
        s_tl_waitOnValue = value;
        handle->fiber = std::move(handle->fiber).resume();
    }
}
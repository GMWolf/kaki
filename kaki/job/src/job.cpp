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
#include <thread>

namespace kaki {

    struct JobWait {
        boost::context::fiber fiber;
        std::atomic<uint64_t>* waitOn;
        uint64_t value;
    };

    struct JobWorker {
        std::vector<JobWait> waitJobs;
        Scheduler::Impl* sc;

        boost::context::fiber nextFiber();

        void run();
    };


    struct Scheduler::Impl {
        std::vector<JobWorker> workers;
        std::vector<std::thread> threads;

        std::mutex jobQueueMutex;
        std::condition_variable jobQueueCondVar;
        std::queue<JobFn> jobQueue;

        std::atomic<bool> shutdown;

        JobFn popNextJob();
        void scheduleJob(JobFn&& fn);
    };

    void Scheduler::Impl::scheduleJob(JobFn &&fn) {
        std::unique_lock lock(jobQueueMutex);
        jobQueue.emplace(std::forward<JobFn>(fn));
        lock.unlock();
        jobQueueCondVar.notify_one();
    }

    void Scheduler::scheduleJob(JobFn &&job) {
        impl->scheduleJob(std::forward<JobFn>(job));
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
        Scheduler::Impl* scheduler;
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



    boost::context::fiber JobWorker::nextFiber() {

        for(size_t i = 0; i < waitJobs.size(); i++)
        {
            if (waitJobs[i].waitOn->load() == waitJobs[i].value) {
                boost::context::fiber fiber = std::move(waitJobs[i].fiber);
                waitJobs.erase(waitJobs.begin() + i);
                return fiber;
            }
        }

        // no ready waiting jobs, get next job in queue
        JobFn job = sc->popNextJob();
        boost::context::fiber fiber([job = std::move(job), this](boost::context::fiber ctx) {
            JobCtxI ctxi {
                    .fiber = std::move(ctx),
                    .scheduler = sc,
            };
            job(JobCtx {
                    .handle = &ctxi,
            });
            return std::move(ctxi.fiber);
        });

        return fiber;
    }

    void JobWorker::run() {
        while(!sc->shutdown) {

            auto fiber = nextFiber();

            fiber = std::move(fiber).resume();

            if ( s_tl_waitOnPtr )
            {
                waitJobs.push_back( JobWait{
                        .fiber = std::move(fiber),
                        .waitOn = s_tl_waitOnPtr,
                        .value = s_tl_waitOnValue,
                });
            }
            s_tl_waitOnPtr = nullptr;
        }
    };

    void Scheduler::shutdownNow() {
        impl->shutdown = true;
        for(int i = 1; i < impl->threads.size(); i++)
        {
            impl->threads[i].join();
        }
    }

    void JobCtx::wait(std::atomic<uint64_t> &atomic, uint64_t value) {
        s_tl_waitOnPtr = &atomic;
        s_tl_waitOnValue = value;
        handle->fiber = std::move(handle->fiber).resume();
    }

    void JobCtx::schedule(JobFn &&function) {
        handle->scheduler->scheduleJob(std::forward<JobFn>(function));
    }

    void Scheduler::run() {

        size_t numThreads = 2;
        impl->workers.resize(numThreads);
        for(int i = 0; i < numThreads; i++) {
            impl->workers[i].sc = impl;
            if (i != 0) impl->threads.emplace_back(&JobWorker::run, &impl->workers.back());
        }

        impl->workers[0].run();

    }
}
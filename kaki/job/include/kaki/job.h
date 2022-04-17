//
// Created by felix on 13/04/2022.
//

#pragma once

#include <functional>
#include <atomic>

namespace kaki {

    struct JobCtx;
    using JobFn = std::function<void(JobCtx)>;

    struct JobCtxI;
    struct JobCtx {
        JobCtxI* handle;
        void wait(std::atomic<uint64_t>& atomic, uint64_t value);
        void schedule(JobFn&& function);
    };


    struct Scheduler {
        struct Impl;
        Impl* impl;

        Scheduler();
        ~Scheduler();
        void scheduleJob(JobFn&& function);

        void run();
        void shutdownNow();
    };

}
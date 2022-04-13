add_library(kaki-job
        include/kaki/job.h
        src/job.cpp
        )
target_link_libraries(kaki-job PRIVATE boost_context boost_lockfree)
target_include_directories(kaki-job
        PUBLIC include
        PRIVATE include/kaki
        )
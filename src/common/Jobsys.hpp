#pragma once

#include <semaphore>
#include <thread>
#include <vector>
#include <deque>


struct Job {
    void (*proc)(void* data);
    void* proc_data;
    std::binary_semaphore is_finished {0};
};

class Jobsys {
public:
    Jobsys();

    void StartJob(Job* job);
    void FinishJob(Job* job);

private:
    std::vector<std::thread> m_threads;

    std::deque<Job*> m_pending_jobs;
    std::counting_semaphore<> m_pending_jobs_sem{0};
    std::mutex  m_pending_jobs_mut;

    friend void JobRunner(Jobsys* jobsys);
};

extern Jobsys* g_jobsys; // surely there's a better design than this...


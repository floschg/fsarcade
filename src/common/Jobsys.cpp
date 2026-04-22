#include "common/Jobsys.hpp"

#include <algorithm>
#include <cassert>

Jobsys* g_jobsys = new Jobsys();

void
JobRunner(Jobsys* jobsys)
{
    while (1) {
        jobsys->m_pending_jobs_sem.acquire();

        jobsys->m_pending_jobs_mut.lock();
        assert(jobsys->m_pending_jobs.size());
        Job* job = jobsys->m_pending_jobs.front();
        jobsys->m_pending_jobs.pop_front();
        jobsys->m_pending_jobs_mut.unlock();

        job->proc(job->proc_data);
        job->is_finished.release();
    }
}

Jobsys::Jobsys()
{
    uint32_t hardware_threads = std::min(4u, std::thread::hardware_concurrency());
    for (uint32_t i = 0; i < hardware_threads; i++) {
        std::thread t(JobRunner, this);
        m_threads.emplace_back(std::move(t));
    }
    printf("jobsys created with %u hardware_threads\n", hardware_threads);
}

void
Jobsys::StartJob(Job* job)
{
    m_pending_jobs_mut.lock();
    m_pending_jobs.emplace_back(job);
    m_pending_jobs_mut.unlock();
    m_pending_jobs_sem.release();
}

void
Jobsys::FinishJob(Job* job)
{
    job->is_finished.acquire();
}


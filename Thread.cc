#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>


sem_t sem_;
std::atomic_int Thread::numCreated_=0;


Thread::Thread(ThreadFunc func,const std::string& name)
    :started_(false)
    ,joined_(false)
    ,func_(std::move(func))
    ,name_(name)  
    ,tid_(0)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::start()
{
    started_=true;
    sem_init(&sem_,false,0);

    thread_=std::shared_ptr<std::thread>(new std::thread([&](){
        tid_=CurrentThread::tid();
        sem_post(&sem_);

        func_();
    }));

    sem_wait(&sem_);
}

void Thread::join()
{
    joined_=true;
    thread_->join();
}


void Thread::setDefaultName()
{
    int num=++numCreated_;
    if(name_.empty())
    {
        char name[128];
        snprintf(name,sizeof(name),"Thread%d",num);
        name_=name;
    }
}
#include "Thread.h"

using namespace base;

/*
 *  构造函数
 */
Thread::Thread(ThreadFunc func)
               :threadFunc_(func),
                threadId_(-1)
{
    threadData_.threadObj_  = this;
    threadData_.threadFunc_ = threadFunc_;
    setStop();
}

/*
 *  析构函数
 *  析构完注意需要唤醒子线程，否则可能阻塞在wait处无法结束
 */
Thread::~Thread()
{
    stopThread();
}

/*
 *  创建子线程
 *  可重复调用
 */
bool Thread::startThread()
{
    // 子线程已启动
    if(isRunning())
        return false;

    int ret = pthread_create(&threadId_,
                             nullptr,
                             entryThread,
                             static_cast<void*>(&threadData_));
    if(ret != 0) // 创建失败
        return false;

    setIdle();
    return true;
}

/*
 *  通知子线程结束
 *  可重复调用
 *  并不会马上结束，而是设置标志位running_为false
 *  由于子线程会阻塞于wait的子线程，需要在调用stopThread()后，再唤醒所有阻塞线程
 */
bool Thread::stopThread()
{
    if(!isRunning())
        return false;

    setStopping(); // 设置threadStatus_为STOPPING，让子线程执行完当前任务后自动退出循环，结束线程
    int ret = pthread_detach(threadId_); // 不会阻塞等待子线程的结束，分离让其结束后自动回收资源
    if(ret == 0)
        return true;
    else
        return false;
}

/****************************************************************************************************************/

/*
 *  子线程入口函数
 *  用entry函数间接调用实际线程函数是为了使用非static成员函数作为线程函数。
 */
void* Thread::entryThread(void* Data)
{
    ThreadData* tmp = static_cast<ThreadData*>(Data);
    tmp->threadFunc_(Data); // 因为ThreadFunc定义为void*(*)(void*)，所以只能传Data不能传tmp
    return nullptr;
}




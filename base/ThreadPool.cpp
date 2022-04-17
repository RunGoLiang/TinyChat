#include "ThreadPool.h"

using namespace base;

/*
 *  构造函数
 */
ThreadPool::ThreadPool(int minThread)
    :running_(false),
    stopping_(false),
    kminThread_(minThread < kmaxTHREAD_ ? minThread : kmaxTHREAD_)
{
    pthread_cond_init(&taskCond_, nullptr);
    pthread_mutex_init(&taskMutex_, nullptr);

    threadData_.threadObj_  = this;
    threadData_.threadFunc_ = std::bind(&ThreadPool::managePool,this,std::placeholders::_1);
}

/*
 *  析构函数
 */
ThreadPool::~ThreadPool()
{
    stopPool();

    pthread_mutex_destroy(&taskMutex_);
    pthread_cond_destroy(&taskCond_);
}

/*
 *  启动线程池
 *  可重复调用
 *  只负责创建manage线程，不负责线程池
 */
bool ThreadPool::startPool()
{
    // 已启动
    if(running_)
        return false;

    // 创建manage线程
    int ret = pthread_create(&poolThread_,
                             nullptr,
                             entryPoolThread,
                             static_cast<void*>(&threadData_));
    if(ret != 0) // 创建失败
        return false;

    running_ = true;
    return true;
}

/*
 *  停止线程池，但不会销毁线程对象
 *  可重复调用
 *  只是通知manage线程停止，会一直等待到停止再退出
 */
bool ThreadPool::stopPool()
{
    // 已停止
    if(!running_)
        return false;

    stopping_ = true; // 通知manage线程关闭
    pthread_detach(poolThread_);

    while(running_); // 等待线程结束
    stopping_ = false; // 便于下一次再启动pool

    return true;
}

/*
 *  添加任务
 *  可跨线程调用
 */
bool ThreadPool::addTask(Task oneTask)
{
    if(taskList_.size() >= kmaxTASK_ || !running_)
        return false;

    pthread_mutex_lock(&taskMutex_);
    taskList_.push(std::move(oneTask));
    pthread_mutex_unlock(&taskMutex_);

    wakeupAllThread();

    return true;
}

/*
 *  唤醒所有阻塞的线程
 */
void ThreadPool::wakeupAllThread()
{
    pthread_cond_broadcast(&taskCond_);
}

/****************************************************************************************************************/

/*
 *  创建并启动新的子线程
 *  不能跨线程调用
 */
bool ThreadPool::createAndStartNewThread()
{
    if(pool_.size() >= kmaxTHREAD_)
        return false;

    std::unique_ptr<Thread> newThread(new Thread(std::bind(&ThreadPool::threadFunc,this,std::placeholders::_1)));
    newThread->startThread();
    pool_.emplace_back(std::move(newThread));
    return true;
}

/*
 *  manage线程主函数
 *  收到stopping通知时，关闭线程池中所有线程，清空线程池队列pool_后再结束manage线程
 */
void *ThreadPool::managePool(void *threadPoolData)
{
    // 创建线程对象，并启动子线程
    while(pool_.size() < kminThread_)
        createAndStartNewThread();

    // manage线程loop
    int bussyNum = 0; // 线程池中正在处理任务的线程数
    int idleNum  = 0; // 线程池中空闲的线程数
    while(!stopping_)
    {
        bussyNum = 0;
        idleNum  = 0;

        // 销毁STOP线程对象
        for(int i=0;i < pool_.size();++i)
        {
            switch (pool_[i]->getStatus())
            {
                case Thread::BUSSY:
                    ++bussyNum;
                    break;
                case Thread::IDEL:
                    ++idleNum;
                    break;
                case Thread::STOP:
                    std::iter_swap(pool_.begin()+i,pool_.end()-1);
                    pool_.pop_back(); // unique_ptr弹出后对象销毁
                    --i;
                    break;
                case Thread::STOPPING:
                    break;
                default:
                    /// FIXME:缺少错误处理
                    break;
            }
        }

        // 全都在忙，且有多余任务，补充线程
        if(idleNum == 0)
        {
            createAndStartNewThread();
        }

        // 停止多余IDLE线程
        bool stopSomething = false;
        for(int i=0;i < pool_.size();++i)
        {
            // 仅保留最少的线程数
            if(bussyNum + idleNum <= kminThread_ || idleNum <= kmaxIDLETHREAD_)
                break;

            if(pool_[i]->isIdle())
            {
                stopSomething = true;
                pool_[i]->stopThread();
                --idleNum;
            }
        }
        if(stopSomething)
            wakeupAllThread();
    }

    // 等待所有子线程停止，销毁线程对象
    while(!pool_.empty())
    {
        for(int i=0;i < pool_.size();++i)
        {
            switch (pool_[i]->getStatus())
            {
                case Thread::BUSSY:
                case Thread::IDEL:
                    pool_[i]->stopThread(); // 通知子线程结束
                    break;
                case Thread::STOP:
                    std::iter_swap(pool_.begin()+i,pool_.end()-1);
                    pool_.pop_back(); // unique_ptr弹出后对象销毁
                    --i;
                    break;
                case Thread::STOPPING:
                    break;
                default:
                    /// FIXME:缺少错误处理
                    break;
            }
        }
        wakeupAllThread(); // 唤醒所有子线程，防止其阻塞在wait处。STOPPING状态的子线程会链式相互唤醒
    }

    running_ = false;
    pthread_exit(nullptr);

    return nullptr;
}

/*
 *  子线程主函数
 */
void* ThreadPool::threadFunc(void *threadData)
{
    Thread::ThreadData *data = static_cast<Thread::ThreadData *>(threadData);
    Thread *thisThread = data->threadObj_; // 线程对象

    while(thisThread->isRunning())
    {
        thisThread->setIdle();
        // 取任务
        pthread_mutex_lock(&taskMutex_);
        while(taskList_.empty())
        {
            pthread_cond_wait(&taskCond_, &taskMutex_);

            if(thisThread->isStopping()) // 有可能是结束线程才被唤醒
            {
                thisThread->setStop();
                pthread_mutex_unlock(&taskMutex_); // 结束线程之前先解锁
                wakeupAllThread(); // 防止抢了其他线程的任务唤醒，所以补一次wakeup；或唤醒其他STOPPING线程
                pthread_exit(nullptr);
            }
            /// FIXME: stop线程后的wakeup信号如果被非STOPPING线程抢到怎么办？
        }
        Task curTask = taskList_.front();
        taskList_.pop();
        pthread_mutex_unlock(&taskMutex_);

        // 执行任务
        if(!thisThread->isStopping()) // 防止STOPPING状态被刷掉
            thisThread->setBussy();
        if(curTask != nullptr)
            curTask();
    }

    thisThread->setStop();
    pthread_exit(nullptr);
}

/*
 *  pool线程入口函数
 */
void *ThreadPool::entryPoolThread(void *Data)
{
    ThreadPoolData* tmp = static_cast<ThreadPoolData*>(Data);
    tmp->threadFunc_(Data); // 因为ThreadFunc定义为void*(*)(void*)，所以只能传Data不能传tmp
    return nullptr;
}
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>

#include "Thread.h"
#include "noncopyable.h"

////////////////////
/// 测试用头文件//////
//#include <iostream>
//#include <string>
////////////////////
////////////////////

namespace base
{

    // 前置声明
    class Thread;

    // 动态线程池，作为任务线程池使用
    class ThreadPool:noncopyable
    {
    public:
        // 打包线程信息
        struct ThreadPoolData
        {
            ThreadPool *threadObj_;  // 线程对象
            ThreadFunc  threadFunc_; // 线程主函数
        };

        /// 可跨线程调用
        explicit ThreadPool(int minThread);
        ~ThreadPool();

        bool startPool();
        bool stopPool();
        bool addTask(Task oneTask);
        void wakeupAllThread();

        /// 不可跨线程调用
        bool createAndStartNewThread();
        void* managePool(void *threadPoolData);
        void* threadFunc(void *threadData);

    private:
        /// 不可跨线程调用
        static void *entryPoolThread(void *);

    private:
        const int kmaxTASK_   = 40;    // 任务队列最大长度
        const int kmaxTHREAD_ = 20;    // 线程池最大容量
        const int kmaxIDLETHREAD_ = 3; //最多容许的idle线程数
        const int kminThread_;         // 线程池中线程对象最小数量

        pthread_cond_t  taskCond_;    // 任务队列的条件变量
        pthread_mutex_t taskMutex_;   // 任务队列的互斥锁

        std::vector<std::unique_ptr<Thread> > pool_;   // 线程对象，必须只能在manage线程中使用
        std::queue<Task> taskList_;   // 任务队列，读写需要加锁

        pthread_t      poolThread_; // 线程池manage线程
        ThreadPoolData threadData_; // 线程信息

        bool running_;  // 线程池是否正在运行
        bool stopping_; // 通知manage线程关闭
    };

} // namespace base

#endif //_THREADPOOL_H

#ifndef _THREAD_H
#define _THREAD_H

#include <functional>
#include <pthread.h>

#include "noncopyable.h"
#include "Atomic.h"

namespace base
{
    using ThreadFunc = std::function<void*(void*)>; // 工作线程主函数
    using Task       = std::function<void(void)>;   // 任务函数

    class Thread:noncopyable
    {
    public:
        // 线程状态
        enum ThreadStatus{
            IDEL = 1,    // 子线程已启动，空闲状态
            BUSSY,       // 子线程正在处理任务
            STOPPING,    // 子线程正在停止
            STOP         // 子线程未启动
        };

        // 打包线程信息
        struct ThreadData
        {
            Thread    *threadObj_;  // 线程对象
            ThreadFunc threadFunc_; // 线程主函数
        };

    public:
        /// 可跨线程调用
        explicit Thread(ThreadFunc func);
        ~Thread(void);

        bool startThread(void);
        bool stopThread(void);

        /// FIXME: 都是系统调用，很耗费资源
        int32_t getStatus(void){ return threadStatus_.get(); }

        bool isRunning(void)   { return threadStatus_.get() == IDEL || threadStatus_.get() == BUSSY; }
        bool isIdle(void)      { return threadStatus_.get() == IDEL; }
        bool isBussy(void)     { return threadStatus_.get() == BUSSY; }
        bool isStopping(void)  { return threadStatus_.get() == STOPPING; }
        bool isStop(void)      { return threadStatus_.get() == STOP; }

        void setIdle(void)     { threadStatus_.getAndSet(IDEL); }
        void setBussy(void)    { threadStatus_.getAndSet(BUSSY); }
        void setStopping(void) { threadStatus_.getAndSet(STOPPING); }
        void setStop(void)     { threadStatus_.getAndSet(STOP); }

    private:
        /// 不可跨线程调用
        static void *entryThread(void *Data);

    private:
        ThreadFunc threadFunc_; // 子线程主函数
        pthread_t  threadId_;   // 子线程id
        ThreadData threadData_; // 线程信息

        AtomicInt32 threadStatus_; // 子线程状态
    };

} // namespace base


#endif //_THREAD_H

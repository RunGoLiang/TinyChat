#ifndef THREAD_H
#define THREAD_H

#include "noncopyable.h"
#include "Atomic.h"
#include "Types.h"

namespace base
{
    /*
     *  线程对象类
     *  调用Thread::startThread()后会执行构造时给定的threadFunc_函数
     * */
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
        ~Thread();

        bool startThread();
        bool stopThread();

        /// FIXME: 都是系统调用，很耗费资源
        int32_t getStatus(){ return threadStatus_.get(); }

        bool isRunning()   { return threadStatus_.get() == IDEL || threadStatus_.get() == BUSSY; }
        bool isIdle()      { return threadStatus_.get() == IDEL; }
        bool isBussy()     { return threadStatus_.get() == BUSSY; }
        bool isStopping()  { return threadStatus_.get() == STOPPING; }
        bool isStop()      { return threadStatus_.get() == STOP; }

        void setIdle()     { threadStatus_.getAndSet(IDEL); }
        void setBussy()    { threadStatus_.getAndSet(BUSSY); }
        void setStopping() { threadStatus_.getAndSet(STOPPING); }
        void setStop()     { threadStatus_.getAndSet(STOP); }

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

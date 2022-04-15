#ifndef _ATOMIC_H
#define _ATOMIC_H

#include <stdint.h>

#include "noncopyable.h"

namespace base
{
    // noncopyable表示不可拷贝，做法是将copy constructor和assignment函数都设为私有
    template<typename T>
    class AtomicIntegerT : noncopyable
    {
    public:
        AtomicIntegerT()
                : value_(0)
        {
        }

        // 原子化地读取value的值，当value是临界资源时，利用get()获取值不需要加锁
        // __sync_val_compare_and_swap是原子化的操作，比较value 的值是否为0，true则设置为0，否则返回value的值
        T get(){ return __sync_val_compare_and_swap(&value_, 0, 0); }
        // 先返回value值，再对value+1
        T getAndAdd(T x){ return __sync_fetch_and_add(&value_, x); }
        T addAndGet(T x){ return getAndAdd(x) + x; }

        T incrementAndGet(){ return addAndGet(1); }
        T decrementAndGet(){ return addAndGet(-1); }

        void add(T x){ getAndAdd(x); }

        void increment(){ incrementAndGet(); }
        void decrement(){ decrementAndGet(); }

        T getAndSet(T newValue){ return __sync_lock_test_and_set(&value_, newValue); }

    private:
        // 当要求使用volatile 声明的变量的值的时候，系统总是重新从它所在的内存读取数据，而不是使用保存在寄存器中的备份。
        // 即使它前面的指令刚刚从该处读取过数据。而且读取的数据立刻被保存
        volatile T value_;
    };

    typedef AtomicIntegerT<int32_t> AtomicInt32;
    typedef AtomicIntegerT<int64_t> AtomicInt64;

}  // namespace base

#endif  // _ATOMIC_H

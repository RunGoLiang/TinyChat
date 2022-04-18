#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace base
{
    // noncopyable表示不可拷贝，做法是将copy constructor和assignment函数都设为非public
    class noncopyable
    {
    public:
        noncopyable(const noncopyable&) = delete; // 禁止编译器自动生成该函数
        void operator=(const noncopyable&) = delete;

    protected:
        noncopyable() = default; // 要求编译器自动生成默认的该函数
        ~noncopyable() = default;
    };

}  // namespace base

#endif  // NONCOPYABLE_H

#ifndef COPYABLE_H
#define COPYABLE_H

namespace base
{
// 一个类继承了copyable就代表这个类的对象可以copy，并且copy的对象与原对象脱离关系
    class copyable
    {
    protected:
        copyable() = default;
        ~copyable() = default;
    };
}

#endif  // COPYABLE_H

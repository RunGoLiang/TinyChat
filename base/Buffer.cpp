#include "Buffer.h"

using namespace base;

const char Buffer::kCRLF[] = "\r\n";

/*
 *  将数据从内核缓冲区读取并保存到buffer的应用层缓冲区，不负责字节序的转换
 */
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extrabuf[65536]; // 64k
    struct iovec vec[2];
    const size_t writable = writableBytes();
    // 第一块缓冲区：buffer本身可写的空间
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    // 第二块缓冲区：额外申请的临时缓冲区
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // readv是将读入数据存储到多个不连续的内存区间，先填满第一个，如果还没读完就填第二个，依次填写直到读取完毕。
    // vec[]数组保存多个内存区间的地址，iovcnt表示有几块不连续区间可供填写
    // 返回值n表示实际读到的字节数

    // 如果Buffer可用空间比extrabuf的64K小，那么Buffer和extrabuf都用上，先填写Buffer再填写extrabuf；
    // 如果Buffer可用空间比extrabuf的64K大，那么只用Buffer；
    // 如果Buffer和extrabuf都用上了还不够，或者 只用了Buffer但写不下，由于是epoll是LT模式，因此这一次读不完就还会触发read event；
    // 极限情况下writable=63K，此时Buffer和extrabuf都用上这一次能读走(128-1)K
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt); // 实际读取字节数n<=writable+64K
    if (n < 0)
    {
        *savedErrno = errno;
    }
        // 若iovcnt=1，n<writable代表buffer就够用，数据已写入Buffer，则更新writerIndex_；
        // 若iovcnt=1，n==writable代表buffer已经写满，由于没用上extrabuf，所以只能先更新writerIndex_，其他的等下次读；
        // 若iovcnt=2，n<=writable代表Buffer够用，数据已写入Buffer，extrabuf没用上；
    else if (static_cast<size_t>(n) <= writable)
    {
        writerIndex_ += n;
    }
        // iovcnt=1时，只使用Buffer，不可能有n>writable，因此iovcnt=1可能出现的两种情况都在上面处理了
        // iovcnt=2，n>writable，数据已写入Buffer和extrabuf，Buffer需要先扩容，然后还有(n - writable)字节需要从extrabuf获取
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}


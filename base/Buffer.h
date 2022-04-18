#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>

#include "Types.h"
#include "copyable.h"

namespace base
{
    /*
     * 应用层缓冲
     * Buffer中的数据都是网络字节序
     * 调用readxxx()、peekxxx()会将Buffer中数据读取并转换成主机字节序，再返回给调用者；
     * 调用appendxxx()时，调用者需要先将主机字节序的数据转换成网络字节序再传入；
     * */
    class Buffer : public copyable
    {
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

        explicit Buffer(size_t initialSize = kInitialSize)
                : buffer_(kCheapPrepend + initialSize),
                  readerIndex_(kCheapPrepend),
                  writerIndex_(kCheapPrepend)
        {
        }

        // 高效的swap，不会真的copy数据
        void swap(Buffer& rhs)
        {
            buffer_.swap(rhs.buffer_);
            std::swap(readerIndex_, rhs.readerIndex_);
            std::swap(writerIndex_, rhs.writerIndex_);
        }

        // 空间大小
        size_t readableBytes() const
        { return writerIndex_ - readerIndex_; }
        size_t writableBytes() const
        { return buffer_.size() - writerIndex_; }
        size_t prependableBytes() const
        { return readerIndex_; }

        // 读的位置
        const char* peek() const
        { return begin() + readerIndex_; }

        // 查找"\r\n"
        const char* findCRLF() const
        {
            const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
            return crlf == beginWrite() ? NULL : crlf;
        }
        const char* findCRLF(const char* start) const
        {
            const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
            return crlf == beginWrite() ? NULL : crlf;
        }

        // 查找“\n”
        const char* findEOL() const
        {
            const void* eol = memchr(peek(), '\n', readableBytes());
            return static_cast<const char*>(eol);
        }
        const char* findEOL(const char* start) const
        {
            assert(peek() <= start);
            assert(start <= beginWrite());
            const void* eol = memchr(start, '\n', beginWrite() - start);
            return static_cast<const char*>(eol);
        }

        // 更新readerIndex_向后移动len
        void retrieve(size_t len)
        {
            assert(len <= readableBytes());
            if (len < readableBytes())
            {
                readerIndex_ += len;
            }
            else
            {
                retrieveAll();
            }
        }
        void retrieveUntil(const char* end)
        {
            assert(peek() <= end);
            assert(end <= beginWrite());
            retrieve(end - peek());
        }
        void retrieveInt64()
        {
            retrieve(sizeof(int64_t));
        }
        void retrieveInt32()
        {
            retrieve(sizeof(int32_t));
        }
        void retrieveInt16()
        {
            retrieve(sizeof(int16_t));
        }
        void retrieveInt8()
        {
            retrieve(sizeof(int8_t));
        }
        void retrieveAll()
        {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }
        std::string retrieveAllAsString()
        {
            return retrieveAsString(readableBytes());
        }
        std::string retrieveAsString(size_t len)
        {
            assert(len <= readableBytes());
            std::string result(peek(), len);
            retrieve(len);
            return result;
        }

        // 向buffer写入网络字节序数据，并更新writerIndex_，如果空间不够会设法提供足够空间来写入
        void append(const char* data, size_t len)
        {
            ensureWritableBytes(len);
            std::copy(data, data+len, beginWrite());
            hasWritten(len);
        }
        void append(const void* data, size_t len)
        {
            append(static_cast<const char*>(data), len);
        }

        // 确保可写空间大于len，如果不够就腾出新空间或申请新空间
        void ensureWritableBytes(size_t len)
        {
            if (writableBytes() < len)
            {
                makeSpace(len);
            }
            assert(writableBytes() >= len);
        }

        char* beginWrite()
        { return begin() + writerIndex_; }
        const char* beginWrite() const
        { return begin() + writerIndex_; }
        void hasWritten(size_t len)
        {
            assert(len <= writableBytes());
            writerIndex_ += len;
        }
        void unwrite(size_t len)
        {
            assert(len <= readableBytes());
            writerIndex_ -= len;
        }

        // 写入单个网络字节序的数据，并更新writerIndex_
        void appendInt64(int64_t x)
        {
            int64_t be64 = htobe64(x);
            append(&be64, sizeof be64);
        }
        void appendInt32(int32_t x)
        {
            int32_t be32 = htobe32(x);
            append(&be32, sizeof be32);
        }
        void appendInt16(int16_t x)
        {
            int16_t be16 = htobe16(x);
            append(&be16, sizeof be16);
        }
        void appendInt8(int8_t x)
        {
            append(&x, sizeof x);
        }

        // “获取”得到单个主机字节序的数据，并更新readerIndex_，类似于queue::front() + queue::pop()
        int64_t readInt64()
        {
            int64_t result = peekInt64();
            retrieveInt64();
            return result;
        }
        int32_t readInt32()
        {
            int32_t result = peekInt32();
            retrieveInt32();
            return result;
        }
        int16_t readInt16()
        {
            int16_t result = peekInt16();
            retrieveInt16();
            return result;
        }
        int8_t readInt8()
        {
            int8_t result = peekInt8();
            retrieveInt8();
            return result;
        }

        // “看一眼”单个主机字节序的数据，不会更新readerIndex_，类似于queue::front()
        int64_t peekInt64() const
        {
            assert(readableBytes() >= sizeof(int64_t));
            int64_t be64 = 0;
            ::memcpy(&be64, peek(), sizeof be64);
            return be64toh(be64);
        }
        int32_t peekInt32() const
        {
            assert(readableBytes() >= sizeof(int32_t));
            int32_t be32 = 0;
            ::memcpy(&be32, peek(), sizeof be32);
            return be32toh(be32);
        }
        int16_t peekInt16() const
        {
            assert(readableBytes() >= sizeof(int16_t));
            int16_t be16 = 0;
            ::memcpy(&be16, peek(), sizeof be16);
            return be16toh(be16);
        }
        int8_t peekInt8() const
        {
            assert(readableBytes() >= sizeof(int8_t));
            int8_t x = *peek();
            return x;
        }

        // 在readerIndex_处前缀单个网络字节序数据
        void prependInt64(int64_t x)
        {
            int64_t be64 = htobe64(x);
            prepend(&be64, sizeof be64);
        }
        void prependInt32(int32_t x)
        {
            int32_t be32 = htobe32(x);
            prepend(&be32, sizeof be32);
        }
        void prependInt16(int16_t x)
        {
            int16_t be16 = htobe16(x);
            prepend(&be16, sizeof be16);
        }
        void prependInt8(int8_t x)
        {
            prepend(&x, sizeof x);
        }
        void prepend(const void* /*restrict*/ data, size_t len)
        {
            assert(len <= prependableBytes());
            readerIndex_ -= len;
            const char* d = static_cast<const char*>(data);
            std::copy(d, d+len, begin()+readerIndex_);
        }

        // 利用swap技法伸缩vector的空间为reserve个字节
        void shrink(size_t reserve)
        {
            Buffer other;
            other.ensureWritableBytes(readableBytes()+reserve);
            other.append(peek(),readableBytes());
            swap(other);
        }

        size_t internalCapacity() const
        {
            return buffer_.capacity();
        }

        // 从socket读取数据保存至buffer
        ssize_t readFd(int fd, int* savedErrno);

    private:
        char* begin()
        { return &*buffer_.begin(); }
        const char* begin() const
        { return &*buffer_.begin(); }

        // 为写入数据腾出空间
        void makeSpace(size_t len)
        {
            // 重新申请空间
            if (writableBytes() + prependableBytes() < len + kCheapPrepend)
            {
                buffer_.resize(writerIndex_+len);
            }
                // 当前空闲空间足够，无需申请新的空间，把数据往前挪
            else
            {
                // move readable data to the front, make space inside buffer
                assert(kCheapPrepend < readerIndex_);
                size_t readable = readableBytes();
                std::copy(begin()+readerIndex_,
                          begin()+writerIndex_,
                          begin()+kCheapPrepend);
                readerIndex_ = kCheapPrepend;
                writerIndex_ = readerIndex_ + readable;
                assert(readable == readableBytes());
            }
        }

    private:
        std::vector<char> buffer_; // buffer用vector管理缓存
        size_t readerIndex_;       // 读的起始位置（读写相对于用户端来说）
        size_t writerIndex_;       // 写的起始位置（读写相对于用户端来说）

        static const char kCRLF[];
    };

}

#endif  // BUFFER_H

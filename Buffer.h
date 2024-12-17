#pragma once

#include <vector>
#include <unistd.h>
#include <string>
#include <algorithm>

class Buffer
{
public:
    static const size_t kCheapPrepend=8;//数据包的头，表示数据的大小
    static const size_t kInitialSize=1024;

    explicit Buffer(size_t InitialSize=kInitialSize)
        :Buffer_(kCheapPrepend+InitialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes()
    {
        return writerIndex_-readerIndex_;
    }

    size_t writableBytes()
    {
        return Buffer_.size()-writerIndex_;
    }

    size_t prependableBytes()
    {
        return readerIndex_;
    }

    const char* peek()const
    {
        return begin()+readerIndex_;
    }

    char* beginWrite()
    {
        return begin()+writerIndex_;
    }

    void retrieve(size_t len)
    {
        if(len<readableBytes())
        {
            readerIndex_+=len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_=writerIndex_=kCheapPrepend;
    }
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }


    void ensureWriteableBytes(size_t len)
    {
        if(writableBytes()<len)
        {
            makeSpace(len);
        }
    }

    void makeSpace(size_t len)
    {
        if(writableBytes()+prependableBytes()<len + kCheapPrepend)
        {
            Buffer_.resize(len + writerIndex_);
        }
        else
        {
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readable+readerIndex_;
        }
    }

    void append(const char* data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    ssize_t readfd(int fd,int* saveErrno);
    ssize_t writefd(int fd,int* saveErrno);
private:
    char* begin()
    {
        return &*Buffer_.begin();
    }

    const char* begin() const
    {
        return &*Buffer_.begin();
    }

    std::vector<char> Buffer_;
    size_t readerIndex_;
    size_t writerIndex_;    

};
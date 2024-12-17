#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>

ssize_t Buffer::readfd(int fd,int* saveErrno)
{
    char extrabuf[65536]={0};
    struct iovec vec[2];
    const size_t writable=writableBytes();
    vec[0].iov_base=begin()+writerIndex_;
    vec[0].iov_len=writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof(extrabuf);

    const int iovcnt=(writable<sizeof(extrabuf))?2:1;
    ssize_t n=::readv(fd,vec,iovcnt);
    if(n<0)
    {
        *saveErrno=errno;
    }
    else if(n<=writable)
    {
        writerIndex_+=n;
    }
    else
    {
        writerIndex_=Buffer_.size();
        append(extrabuf,n-writable);
    }
    return n;
}

ssize_t Buffer::writefd(int fd,int * saveErrno)
{
    ssize_t n=::write(fd,peek(),readableBytes());
    if(n<0)
    {
        *saveErrno=errno;
    }
    return n;
}
/* 使用标准输入输出的方式封装了socket，定义了BaseTCPStreamBuf,
 * BaseTCPStream和BaseTCPUtils. */

#include "SocketStreamBase.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

namespace myrpc {

BaseTcpStreamBuf::BaseTcpStreamBuf(size_t buf_size) 
    : buf_size_(buf_size) {
    char *getbuf = new char[buf_size_];
    char *putbuf = new char[buf_size_];

    setg(getbuf, getbuf, getbuf);
    setp(putbuf, putbuf + buf_size_);
}

/* eback() returns the beginning pointer for the input sequence.
 * gptr() returns the next pointer for the input sequence.
 * egptr() returns the end pointer for the input sequence.
 * 
 * pbase() returns the beginning pointer for the output sequence.
 * pptr() returns the next pointer for the output sequence.
 * epptr() returns the end pointer for the output sequence.
 * 
 * */
BaseTcpStreamBuf::~BaseTcpStreamBuf() 
{
    delete[] eback();
    delete[] pbase();
}

//从socket中接受数据并重置指针
int BaseTcpStreamBuf::underflow() 
{
    int ret = precv(eback(), buf_size_, 0);
    if (ret > 0) {
        setg(eback(), eback(), eback() + ret);
        return traits_type::to_int_type(*gptr());
    }
    else {
        return traits_type::eof();
    }
}

//缓存区已满，经数据发送
int BaseTcpStreamBuf::sync() 
{
    int sent = 0;
    int total = pptr() - pbase();
    while (sent < total) {
        int ret = psend(pbase() + sent, total -sent, 0);
        if (ret > 0) 
            sent = sent + ret;
        else 
            return -1;
    }

    setp(pbase(), pbase() + buf_size_);
    //重置pptr()到起始位置
    pbump(0);

    return  0;
}

//将所有数据发送并清空缓存区
int BaseTcpStreamBuf::overflow(int c) 
{
    if (sync() == -1) {
        return traits_type::eof();
    }
    else {
        if  (!traits_type::eq_int_type(c, traits_type::eof())) {
            sputc(traits_type::to_char_type(c));
        }
        return traits_type::not_eof(c);
    }
}


BaseTcpStream::BaseTcpStream(size_t buf_size)
    : std::iostream(NULL), buf_size_(buf_size) {
        
}

BaseTcpStream::~BaseTcpStream() 
{
    delete rdbuf();
}

void BaseTcpStream::NewRdbuf(BaseTcpStreamBuf *buf) 
{
    std::streambuf *old = rdbuf(buf);
    delete old;
}

//获取与套接字关联的外地协议地址
bool BaseTcpStream::GetRemoteHost(char *ipaddress, size_t size, int *port) 
{
    struct sockaddr_in addr;
    socklen_t slen = sizeof(addr);
    bzero(&addr, sizeof(addr));

    int ret;
    if ((ret = getpeername(Sockfd(), (struct sockaddr *) &addr, &slen)) == 0) {
        inet_ntop(AF_INET, &addr, ipaddress, size);
        if (port != NULL) 
            *port = ntohs(addr.sin_port);
    }

    return ret == 0; 
}

std::istream &BaseTcpStream::getlineWithTrimRight(char *line, size_t size) 
{
    if (getline(line, size).good()) {
        for (char *pos = line + gcount() - 1; pos >= line; pos--) {
            if (*pos == '\0' || *pos == '\r' || *pos == '\n') {
                *pos = '\0';
            }
            else 
                break;
        }
    }

    return *this;
}


//设置文件描述符为非阻塞或者阻塞
bool BaseTcpUtils::SetNonBlock(int fd, bool flag) 
{
    int ret = 0;

    int temp = fcntl(fd, F_GETFL, 0);

    if (flag) {
        ret = fcntl(fd, F_SETFL, temp | O_NONBLOCK);
    }
    else {
        ret = fcntl(fd, F_SETFL, temp & (~O_NONBLOCK));
    }

    if (ret != 0) {
        //myrpc::log
    }

    return ret == 0;
}

//设置为nodelay
bool BaseTcpUtils::SetNoDelay(int fd, bool flag) 
{
    int temp = flag ? 1 : 0;

    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &temp, sizeof(temp));

    if (ret != 0) {
        // myrpc::log
    }

    return ret == 0;
}

}
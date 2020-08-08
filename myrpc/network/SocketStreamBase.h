/* 使用标准输入输出的方式封装了socket，定义了BaseTCPStreamBuf,
 * BaseTCPStream和BaseTCPUtils. */

#pragma once

#include <iostream>

//命名空间myrpc
namespace myrpc {
    
enum SocketStreamError {
    SocketStreamError_Refused = -1,
    SocketStreamError_Timeout = -202,
    SocketStreamError_Normal_Closed = -303,
};

//定义为抽象类，BlockTcpStreamBuf继承自该类
class BaseTcpStreamBuf : public std::streambuf {
public:
    BaseTcpStreamBuf(size_t buf_size);
    virtual ~BaseTcpStreamBuf();

    //从socket中接受数据并重置指针
    int underflow();

    //将所有数据发送并清空缓存区
    int overflow(int c = traits_type::eof());
    //缓存区已满，经数据发送
    int sync();

protected:
    virtual ssize_t precv(void *buf, size_t len, int flags) = 0;
    virtual ssize_t psend(const void *buf, size_t len, int flags) = 0;

    const size_t buf_size_;
};

class BaseTcpStream : public std::iostream {
public:
    BaseTcpStream(size_t buf_size = 4096);

    virtual ~BaseTcpStream();

    virtual bool SetTimeout(int socket_timeout_ms) = 0;
    
    void NewRdbuf(BaseTcpStreamBuf *buf);

    bool GetRemoteHost (char *ipaddress, size_t size, int *port = NULL);

    std::iostream &getlineWithTrimRight(char *line, size_t size);

    virtual int LasrError() = 0;

protected:
    virtual int Sockfd() = 0;
    const size_t buf_size_;
};

class BaseTcpUtils {
public:

    //set socket nonblock
    static bool SetNonBlock(int fd, bool flag);

    //set socket nondelay
    static bool SetNoDelay(int fd, bool flag);
};

}
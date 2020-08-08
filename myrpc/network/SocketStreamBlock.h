/* 定义了BlockTcpStreamBuf, BlockTcpStream和BlockTcpUtils.
 * BlockTcpStreamBuf负责网络IO
 * BlockTcpStream实现了阻塞TCP流
 * BlockTcpUtils封装了网络连接的操作
 *  */

#pragma once 

#include <iostream>
#include "SocketStreamBase.h"

namespace myrpc {

class BlockTcpStreamBuf : public BaseTcpStreamBuf {
public:
    BlockTcpStreamBuf(int socket, size_t buf_size);
    virtual ~BlockTcpStreamBuf();

    ssize_t precv(void *buf, size_t len, int flags);
    ssize_t psend(const void *buf, size_t len, int flags);

private:
    int socket_;
};

class BlockTcpStream : public BaseTcpStream {
public:
    BlockTcpStream(size_t buf_size = 1024);
    ~BlockTcpStream();

    void Attach(int socket);

    bool SetTimeout(int socket_timeout_ms);

    int Sockfd();

    int LastError();

private:
    int socket_;
};

class BlockTcpUtils {
public:
    static bool Open(BlockTcpStream *stream, const char *ipaddress, unsigned short port, 
    int connect_timeout_ms, const char *bind_addr, int bind_port);

    static bool Listen(int *listenfd, const char *ipaddress, unsigned short port);

    static int Poll(int fd, int events, int *revents, int timeout_ms);
};

}
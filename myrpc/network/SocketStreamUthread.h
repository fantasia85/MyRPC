/* 使用标准输入输出的方式封装了socket.
 * UThreadTcpStreamBuf继承自BaseTcpStreamBuf.
 * UThreadTcpStream继承自BaseTcpStream.
 * */

#pragma once

#include "SocketStreamBase.h"
#include "UThreadEpoll.h"

#include <iostream>

namespace myrpc {

class UThreadTcpStreamBuf : public BaseTcpStreamBuf {
public:
    UThreadTcpStreamBuf(UThreadSocket_t *socket, size_t buf_size);
    virtual ~UThreadTcpStreamBuf();

    ssize_t precv(void *buf, size_t len, int flags);
    ssize_t psend(const void *buf, size_t len, int flags);

private:
    UThreadSocket_t *uthread_socket_;
};

class UThreadTcpStream : public BaseTcpStream {
public:
    UThreadTcpStream(size_t buf_size = 1024);
    ~UThreadTcpStream();

    void Attach(UThreadSocket_t *socket);

    UThreadSocket_t *DetachSocket();

    bool SetTimeout(int socket_timeout_ms);

    int SocketFd();
    
    int LastError();

private:
    UThreadSocket_t *uthread_socket_;
};

class UThreadTcpUtils {
public:
    static bool Open(UThreadEpollScheduler *tt, UThreadTcpStream *stream, const char *ip,
        unsigned short port, int connect_timeout_ms);
};

}
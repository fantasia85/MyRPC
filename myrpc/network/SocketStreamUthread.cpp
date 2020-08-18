/* 使用标准输入输出的方式封装了socket.
 * UThreadTcpStreamBuf继承自BaseTcpStreamBuf.
 * UThreadTcpStream继承自BaseTcpStream.
 * */

#include "SocketStreamUthread.h"
#include <assert.h>
#include <string.h>

namespace myrpc {

UThreadTcpStreamBuf::UThreadTcpStreamBuf(UThreadSocket_t *socket, size_t buf_size) 
    : BaseTcpStreamBuf(buf_size), uthread_socket_(socket) {

}

UThreadTcpStreamBuf::~UThreadTcpStreamBuf() {

}

ssize_t UThreadTcpStreamBuf::precv(void *buf, size_t len, int flags) {
    return UThreadRecv(*uthread_socket_, buf, len, flags);
}

ssize_t UThreadTcpStreamBuf::psend(const void *buf, size_t len, int flags) {
    return UThreadSend(*uthread_socket_, buf, len, flags);
}

UThreadTcpStream::UThreadTcpStream(size_t buf_size)
    : BaseTcpStream(buf_size), uthread_socket_(nullptr) {

}

UThreadTcpStream::~UThreadTcpStream() {
    if (uthread_socket_ != nullptr) {
        UThreadClose(*uthread_socket_);
        free(uthread_socket_);
    }
}

void UThreadTcpStream::Attach(UThreadSocket_t *socket) {
    NewRdbuf(new UThreadTcpStreamBuf(socket, buf_size_));
    uthread_socket_ = socket;
}

UThreadSocket_t *UThreadTcpStream::DetachSocket() {
    auto socket = uthread_socket_;
    uthread_socket_ = nullptr;
    return socket;
}

bool UThreadTcpStream::SetTimeout(int socket_timeout_ms) {
    UThreadSetSocketTimeout(*uthread_socket_, socket_timeout_ms);
    return true;
}

int  UThreadTcpStream::SocketFd() {
    return UThreadSocketFd(*uthread_socket_);
}

int UThreadTcpStream::LastError() {
    if (errno == ETIMEDOUT) {
        return SocketStreamError_Timeout;
    }
    else if (errno == EINVAL || errno == ECONNREFUSED) 
        return  SocketStreamError_Refused;
    else   
        return  SocketStreamError_Normal_Closed;

    return -1;
}

bool UThreadTcpUtils::Open(UThreadEpollScheduler *tt, UThreadTcpStream *stream, const char *ip,
    unsigned short port, int connect_timeout_ms) {
    int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    UThreadSocket_t *socket = tt->CreateSocket(fd);
    assert(socket != NULL);
    UThreadSetConnectTimeout(*socket, connect_timeout_ms);

    struct sockaddr_in in_addr;
    bzero(&in_addr, sizeof(in_addr));

    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = inet_addr(ip);
    in_addr.sin_port = htons(port);

    int ret = UThreadConnect(*socket, (struct sockaddr *) &in_addr, sizeof(in_addr));
    if (ret == 0)
        stream->Attach(socket);
    else {
        UThreadClose(*socket);
        free(socket);
    }

    return  ret == 0;
}

}
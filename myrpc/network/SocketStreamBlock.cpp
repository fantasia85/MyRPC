/* 定义了BlockTcpStreamBuf, BlockTcpStream和BlockTcpUtils.
 * BlockTcpStreamBuf负责网络IO
 * BlockTcpStream实现了阻塞TCP流
 * BlockTcpUtils封装了网络连接的操作
 *  */

#include "SocketStreamBlock.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>

namespace myrpc {

BlockTcpStreamBuf::BlockTcpStreamBuf(int socket, size_t buf_size)
    : BaseTcpStreamBuf(buf_size), socket_(socket) {

}

BlockTcpStreamBuf::~BlockTcpStreamBuf() {

}

ssize_t BlockTcpStreamBuf::precv(void *buf, size_t len, int flags) {
    return recv(socket_, buf, len, flags);
}

ssize_t BlockTcpStreamBuf::psend(const void *buf, size_t len, int flags) {
    return send(socket_, buf, len, flags);
}


BlockTcpStream::BlockTcpStream(size_t buf_size) 
    : BaseTcpStream(buf_size), socket_(-1) {

}

BlockTcpStream::~BlockTcpStream() {
    if (socket_ >= 0) {
        close(socket_);
    }
}

//将流缓冲区与套接字绑定
void BlockTcpStream::Attach(int socket) {
    NewRdbuf(new BlockTcpStreamBuf(socket, buf_size_));
    socket_ = socket;
}

bool BlockTcpStream::SetTimeout(int socket_timeout_ms) {
    if (Sockfd() < 0)
        return false;

    struct timeval tout; 
    tout.tv_sec = socket_timeout_ms / 1000;
    tout.tv_usec = (socket_timeout_ms % 1000) * 1000;

    if (setsockopt(Sockfd(), SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout)) < 0) {
        //myrpc::log
        return false;
    }

    if (setsockopt(Sockfd(), SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout)) < 0) {
        //myrpc::log
        return false;
    }

    return true;
}

int BlockTcpStream::Sockfd() {
    return socket_;
}

int BlockTcpStream::LastError() {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return SocketStreamError_Timeout;
    }
    else 
        return SocketStreamError_Refused;
}


//Open由客户端调用
bool BlockTcpUtils::Open(BlockTcpStream *stream, const char *ipaddress, unsigned short port, 
    int connect_timeout_ms, const char *bind_addr, int bind_port) {
    if (inet_addr(ipaddress) == INADDR_NONE) {
        //myrpc::log
        return false;
    }

    if (bind_addr != NULL && inet_addr(bind_addr) == INADDR_NONE) {
        // myrpc::log
        return false;
    }

    int sockfd = - 1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
        //myrpc::log
        return false;
    }

    struct sockaddr_in in_addr;
    bzero(&in_addr, sizeof(in_addr));
    in_addr.sin_family = AF_INET;

    if (bind_addr != NULL && *bind_addr != '\0') {
        in_addr.sin_addr.s_addr = inet_addr(bind_addr);
        in_addr.sin_port = bind_port;

        if (bind(sockfd, (struct sockaddr *) &in_addr, sizeof(in_addr)) < 0) {
            //myrpc::log
        }
    }

    in_addr.sin_addr.s_addr = inet_addr(ipaddress);
    in_addr.sin_port = htons(port);

    BaseTcpUtils::SetNonBlock(sockfd, true);

    int error = 0;
    int ret = connect(sockfd, (struct sockaddr *) &in_addr, sizeof(in_addr));

    if (ret != 0 && ((errno != EINPROGRESS) && (errno != EAGAIN))) {
        //myrpc::log
        error = -1;
    }

    if (error == 0 && ret != 0) {
        int revents = 0;

        ret = Poll(sockfd, POLLOUT, &revents, connect_timeout_ms);
        if (POLLOUT & revents) {
            socklen_t len = sizeof(error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) &error, &len) < 0) {
                //myrpc::log
                error = -1;
            }
        }
        else 
            error = -1;
    }

    if (error == 0) {
        if (BaseTcpUtils::SetNonBlock(sockfd, false) && BaseTcpUtils::SetNoDelay(sockfd, true)) {
            stream->Attach(sockfd);
        } 
        else {
            //myrpc::log
            error = -1;
            close(sockfd);
        }
    }
    else
        close(sockfd);

    return error == 0;
}

//Listen由服务端调用
bool BlockTcpUtils::Listen(int *listenfd, const char *ipaddress, unsigned short port) {
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        //myrpc::log
        return  false;
    }

    int ret = 0;

    int flags = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &flags, sizeof (flags)) < 0) {
        //myrpc::log
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (*ipaddress != '\0') {
        addr.sin_addr.s_addr = inet_addr(ipaddress);
        if (addr.sin_addr.s_addr == INADDR_NONE) {
            //myrpc::log
            ret = -1;
        }
    }

    if (ret == 0) {
        if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            //myrpc::log
            ret = -1;
        }
    }

    if (ret == 0) {
        if (listen(sockfd, 1024) < 0) {
            //myrpc::log
            ret = -1;
        }
    }

    if (ret != 0 && sockfd >= 0) 
        close(sockfd);

    if (ret == 0) {
        *listenfd = sockfd;
        //myrpc::log
    }

    return ret == 0;
}

int BlockTcpUtils::Poll(int fd, int events, int *revents, int timeout_ms) {
    int ret = -1;

    struct pollfd pfd;
    bzero(&pfd, sizeof(pfd));

    pfd.fd = fd;
    pfd.events = events;

    errno = 0;

    //retry again for EINTR
    for (int i = 0; i < 2; i++) {
        ret = poll(&pfd, 1, timeout_ms);
        if (ret == -1 && EINTR == errno) 
            continue;
        break;
    }

    if (ret == 0)
        errno = ETIMEDOUT;

    *revents = pfd.revents;

    return ret;
}

}

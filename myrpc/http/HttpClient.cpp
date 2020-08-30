/* 封装了客户端的HTTP请求，包含Get，Post和Head请求. */

#include "HttpClient.h"
#include "HttpMsg.h"
#include "HttpMsgHandler.h"
#include "HttpProtocol.h"
#include "../network/SocketStreamBase.h"

namespace myrpc {

int HttpClient::Get(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp) {
    int ret = HttpProtocol::SendReqHeader(socket, "Get", req);

    if (ret == 0) {
        ret = HttpProtocol::RecvRespStartLine(socket, resp);
        if (ret == 0) 
            ret = HttpProtocol::RecvRespStartLine(socket, resp);
        if (ret == 0 && SC_NOT_MODIFIED != resp->status_code()) 
            ret = HttpProtocol::RecvBody(socket, resp);
    }

    return static_cast<int> (ret);
}   

int HttpClient::Post(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp) {
    PostStat stat;
    int ret = Post(socket, req, resp, &stat);
    return ret;
}

int HttpClient::Post(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp, 
    PostStat *post_stat) {
    int ret = HttpProtocol::SendReqHeader(socket, "POST", req);

    if (ret == 0) {
        socket << req.content();
        if (!socket.flush().good())
            ret = static_cast<int> (socket.LastError());
    } 
    else {
        if (SocketStreamError_Normal_Closed != ret) {
            post_stat->send_error_ = true;
            //log
        }
        return static_cast<int> (ret);
    }

    if (ret == 0) {
        ret = HttpProtocol::RecvRespStartLine(socket, resp);
        if (ret == 0) 
            ret = HttpProtocol::RecvHeaders(socket, resp);

        if (ret == 0 && SC_NOT_MODIFIED != resp->status_code()) {
            ret = HttpProtocol::RecvBody(socket, resp);
        }

        if (ret != 0 && SocketStreamError_Normal_Closed != ret) {
            post_stat->recv_error_  = true;
        }
    }
    else {
        if (SocketStreamError_Normal_Closed != ret) {
            post_stat->send_error_ = true;
            //log
        }
    }

    return static_cast<int>(ret);
}

int HttpClient::Head(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp) {
    int ret = HttpProtocol::SendReqHeader(socket, "HEAD", req);

    if (ret == 0) 
        ret = HttpProtocol::RecvRespStartLine(socket, resp);

    if (ret == 0)
        ret = HttpProtocol::RecvHeaders(socket, resp);

    return static_cast<int>(ret);
}

}
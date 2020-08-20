/* 用于修改请求和响应的头部.
 * 发送请求头部.
 * 接受请求行，头部和实体以及接受请求和响应.
 *  */ 

#pragma once 

namespace myrpc {

class BaseTcpStream;

class HttpMessage;
class HttpRequest;
class HttpResponse;

class HttpProtocol {
public:
    enum {
        MAX_RECV_LEN = 8192
    };

    enum {
        SC_NOT_MODIFIED = 304
    };

    enum class Direction {
        NONE = 0,
        REQUEST,
        RESPONSE,
        MAX,
    };

    static void FixRespHeaders(const HttpRequest &req, HttpResponse *resp);
    static void FixRespHeaders(bool keep_alive, const char *version, HttpResponse *resp);
    static int SendReqHeader(BaseTcpStream &socket, const char *method, const HttpRequest &req);
    static int RecvRespStartLine(BaseTcpStream &socket,  HttpResponse *resp);
    static int RecvReqStartLine(BaseTcpStream &socket, HttpRequest *req);
    static int RecvHeaders(BaseTcpStream &socket, HttpMessage *msg);
    static int RecvBody(BaseTcpStream &socket, HttpMessage *msg);
    static int RecvReq(BaseTcpStream &socket, HttpRequest *req);
    static int RecvResp(BaseTcpStream &socket, HttpResponse *resp);

};

}
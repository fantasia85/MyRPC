/* 封装了客户端的HTTP请求，包含Get，Post和Head请求. */

#pragma once 

namespace myrpc {

class BaseTcpStream;
class HttpRequest;
class HttpResponse;

class HttpClient {
public:
    struct PostStat {
        bool send_error_ = false;
        bool recv_error_ = false;

        PostStat() = default;

        PostStat(bool send_error, bool recv_error) 
            : send_error_(send_error), recv_error_(recv_error) {

        } 
    };

    enum {
        SC_NOT_MODIFIED = 304
    };

    static int Get(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp);

    static int Post(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp,
        PostStat *post_stat);

    static int Post(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp);

    static int Head(BaseTcpStream &socket, const HttpRequest &req, HttpResponse *resp);

private:
    HttpClient();
};

}
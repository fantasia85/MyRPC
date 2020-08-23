/* 用于修改请求和响应的头部.
 * 发送请求头部.
 * 接受请求行，头部和实体以及接受请求和响应.
 *  */ 

#include "HttpProtocol.h"
#include "HttpMsg.h"
#include "../network/SocketStreamBase.h"
#include <cstring>

namespace {

//用于分割字符串
char *SeparateStr(char **s, const char *del) {
    char *d, *tok;

    if (!s || !*s)
        return nullptr;

    tok = *s;
    d = strstr(tok, del);

    if (d) {
        *s = d + strlen(del);
        *d = '\0';
    }
    else 
        *s = nullptr;

    return tok;
}

//编码URL
void URLEncode(const char *source, char *dest, size_t length) {
    const char urlencstring[] = "0123456789abcedf";

    const char *p = source;
    char *q = dest;
    size_t n = 0;

    for ( ; *p && n < length; p++, q++, n++) {
        if (isalnum((int) *p)) {
            *q = *p;
        }
        else if (*p == ' ') 
            *q = '+';
        else {
            if (n > length - 3) {
                q++;
                break;
            }

            *q++ = '%';
            int digit = *p >> 4;
            *q++ = urlencstring[digit];
            digit = *p & 0xf;
            *q = urlencstring[digit];
            n += 2;
        }
    }

    *q = 0;
}

}

namespace myrpc {

void HttpProtocol::FixRespHeaders(bool keep_alive, const char *version, HttpResponse *resp) {
    char buffer[256] = {0};

    //检查keep-alive头部
    if (keep_alive) {
        if (resp->GetHeaderValue(HttpMessage::HEADER_CONNECTION) == nullptr) {
            resp->AddHeader(HttpMessage::HEADER_CONNECTION, "Keep-Alive");
        }
    }

    //检查数据头部
    resp->RemoveHeader(HttpMessage::HEADER_DATE);
    time_t t_time = time(nullptr);
    struct tm tm_time;
    gmtime_r(&t_time, &tm_time);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
    resp->AddHeader(HttpMessage::HEADER_DATE, buffer);

    //检查服务头部
    resp->RemoveHeader(HttpMessage::HEADER_SERVER);
    resp->AddHeader(HttpMessage::HEADER_SERVER, "http/myrpc");

    //设置版本 
    resp->set_version(version);
}

void HttpProtocol::FixRespHeaders(const HttpRequest &req, HttpResponse *resp) {
    FixRespHeaders(req.keep_alive(), req.version(), resp);
}

int HttpProtocol::SendReqHeader(BaseTcpStream &socket, const char *method, const HttpRequest &req) {
    std::string  url;

    if (req.GetParamCount() > 0) {
        url.append(req.uri());
        url.append("?");

        char tmp[1024] = {0};
        for (size_t i = 0; i < req.GetParamCount(); i++) {
            if (i > 0)
                url.append("&");
            URLEncode(req.GetParamName(i), tmp, sizeof(tmp) - 1);
            url.append(tmp);
            url.append("=");
            URLEncode(req.GetParamValue(i), tmp, sizeof(tmp) - 1);
            url.append(tmp);
        }
    }

    socket << method << " " << (url.size() > 0 ? url.c_str() : req.uri()) 
            << " " << req.version() << "\r\n";

    for (size_t i = 0; i < req.GetHeaderCount(); ++i) {
        const char *name = req.GetHeaderName(i);
        const char *val = req.GetHeaderValue(i);

        socket << name << ": " << val << "\r\n";
    }

    if (req.content().size() > 0) {
        if (req.GetHeaderValue(HttpMessage::HEADER_CONTENT_LENGTH) == nullptr) {
            socket << HttpMessage::HEADER_CONTENT_LENGTH << ": "
                    << req.content().size() << "\r\n";
        }
    }

    socket << "\r\n";

    if (req.content().size() == 0) {
        if (socket.flush().good()) {
            return 0;
        }
        else
            return  static_cast<int> (socket.LastError());
    }

    return 0;
}

}

/* 用于修改请求和响应的头部.
 * 发送请求头部.
 * 接受请求行，头部和实体以及接受请求和响应.
 *  */ 

#include "HttpProtocol.h"
#include "HttpMsg.h"
#include "../network/SocketStreamBase.h"
#include <cstring>
#include <assert.h>

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

int HttpProtocol::RecvRespStartLine(BaseTcpStream &socket, HttpResponse *resp) {
    char line[1024] = {0};
    
    bool is_good = socket.getlineWithTrimRight(line,  sizeof(line)).good();

    if (is_good) {
        if (strncasecmp(line, "HTTP", strlen("HTTP")) == 0) {
            char *pos = line;
            char *first = SeparateStr(&pos, " ");
            char *second = SeparateStr(&pos, " ");

            if (first != nullptr) 
                resp->set_version(first);
            if (second != nullptr) 
                resp->set_status_code(atoi(second));
            if (pos != nullptr)
                resp->set_reason_phrase(pos);
        }
        else {
            is_good = false;
            //log
        }
    }

    if (is_good) 
        return 0;
    else {
        //log
        return static_cast<int> (socket.LastError());
    }
}

int HttpProtocol::RecvReqStartLine(BaseTcpStream &socket, HttpRequest *req) {
    char line[1024] = {0};

    bool is_good = socket.getlineWithTrimRight(line, sizeof(line)).good();
    if (is_good) {
        char *pos = line;
        char *first = SeparateStr(&pos, " ");
        char *second = SeparateStr(&pos, " ");

        if (first != nullptr) 
            req->set_method(first);
        if (second != nullptr) 
            req->set_uri(second);
        if (pos != nullptr)
            req->set_version(pos);
    }
    else {
        //log
    }

    if (is_good)
        return 0;
    else 
        return static_cast<int> (socket.LastError());
}

int HttpProtocol::RecvHeaders(BaseTcpStream &socket, HttpMessage *msg) {
    bool is_good = false;

    char *line = (char *) malloc (MAX_RECV_LEN);
    assert(line != nullptr);

    std::string multi_line;
    char *pos = nullptr;

    do {
        is_good = socket.getlineWithTrimRight(line, MAX_RECV_LEN).good();
        if (!is_good)
            break;

        if ((!isspace(*line)) || *line == '\0') {
            if (multi_line.size() > 0) {
                char *header = (char *) multi_line.c_str();
                pos = header;
                SeparateStr(&pos, ":");
                for ( ; pos != nullptr && *pos != '\0' && isspace(*pos); ) 
                    pos++;
                msg->AddHeader(header, pos == nullptr ? "" : pos);
            }
            multi_line.clear();
        }

        for (pos = line; *pos != '\0' && isspace(*pos); )
            pos++;
        
        if (*pos != '\0')
            multi_line.append(pos);
    } while (is_good && *line != '\0');

    free(line);
    line = nullptr;

    if (is_good) {
        return 0;
    }
    else 
        return static_cast<int> (socket.LastError());
}

int HttpProtocol::RecvBody(BaseTcpStream &socket, HttpMessage *msg) {
    bool is_good = true;

    const char *encoding = msg->GetHeaderValue(HttpMessage::HEADER_TRANSFER_ENCODING);

    char *buff = (char *) malloc (MAX_RECV_LEN);
    assert(buff != nullptr);

    if (encoding != nullptr && strcasecmp(encoding, "chunker") == 0) {
        for ( ; is_good; ) {
            is_good = socket.getline(buff, MAX_RECV_LEN).good();
            if (!is_good)
                break;

            int size = static_cast<int> (strtol(buff, nullptr, 16));
            if (size > 0) {
                for ( ; size > 0; ) {
                    int read_len = size > MAX_RECV_LEN ? MAX_RECV_LEN :size;
                    is_good = socket.read(buff, read_len).good();
                    if (is_good) {
                        size -= read_len;
                        msg->AppendContent(buff, read_len);
                    }
                    else 
                        break;
                }
                is_good = socket.getline(buff, MAX_RECV_LEN).good();
            }
            else
                break;
        }
    }
    else {
        const char *content_length = msg->GetHeaderValue(HttpMessage::HEADER_CONTENT_LENGTH);

        if (content_length != nullptr) {
            int size = atoi(content_length);

            for ( ; size > 0 && is_good; ) {
                int read_len = size > MAX_RECV_LEN ? MAX_RECV_LEN : size;
                is_good = socket.read(buff, read_len).good();
                if (is_good) {
                    size -= read_len;
                    msg->AppendContent(buff, read_len);
                }
                else 
                    break;
            }
        }
        else if (HttpMessage::Direction::RESPONSE == msg->direction()) {
            for ( ; is_good; )  {
                is_good = socket.read(buff, MAX_RECV_LEN).good();
                if (socket.gcount() > 0) 
                    msg->AppendContent(buff, socket.gcount());
            }

            if (socket.eof())
                is_good = true;
        }
    }

    free(buff);

    if (is_good)
        return 0;
    else 
        return  static_cast<int> (socket.LastError());
}

int HttpProtocol::RecvReq(BaseTcpStream &socket,  HttpRequest *req) {
    int ret = RecvReqStartLine(socket, req);

    if (ret == 0) 
        ret = RecvHeaders(socket, req);

    if (ret == 0)
        ret = RecvBody(socket, req);

    return ret;
}

int HttpProtocol::RecvResp(BaseTcpStream &socket, HttpResponse *resp) {
    int ret = RecvRespStartLine(socket, resp);

    if (ret == 0) 
        ret = RecvHeaders(socket, resp);

    if (ret == 0 && SC_NOT_MODIFIED != resp->status_code()) {
        ret = RecvBody(socket, resp);
    }

    return ret;
}

}

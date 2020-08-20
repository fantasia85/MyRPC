/* 封装了http的请求和响应.
 * 定义了HttpMessage, HttpRequest和HttpResponse.
 *  */

#include "HttpMsg.h"
#include "HttpProtocol.h"

namespace myrpc {

const char *HttpMessage::HEADER_CONTENT_LENGTH = "Content-Length";
const char *HttpMessage::HEADER_CONTENT_TYPE = "Content-Type";
const char *HttpMessage::HEADER_CONNECTION = "Connection";
const char *HttpMessage::HEADER_PROXY_CONNECTION = "Proxy-Connection";
const char *HttpMessage::HEADER_TRANSFER_ENCODING = "Transfer-Encoding";
const char *HttpMessage::HEADER_DATE = "Date";
const char *HttpMessage::HEADER_SERVER = "Server";

const char *HttpMessage::HEADER_X_MYRPC_RESULT = "X-MYRPC-Result";

int HttpMessage::ToPb(google::protobuf::Message *const message) const {
    if (!message->ParseFromString(content()))
        return -1;
    
    return 0;
}

}
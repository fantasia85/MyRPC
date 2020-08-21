/* 封装了http的请求和响应.
 * 定义了HttpMessage, HttpRequest和HttpResponse.
 *  */

#include "HttpMsg.h"
#include "HttpProtocol.h"
#include "../rpc/myrpc.pb.h"

#include <cstring>

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

int HttpMessage::FromPb(const google::protobuf::Message &message) {
    if (!message.SerializeToString(mutable_content()))
        return -1;

    return 0;
}

size_t HttpMessage::size() const {
    return content().size();
}

void HttpMessage::AddHeader(const char *name, const char *value) {
    header_name_list_.push_back(name);
    header_value_list_.push_back(value);
}

void HttpMessage::AddHeader(const char *name, int value) {
    char tmp[32] {0};
    snprintf(tmp, sizeof(tmp), "%d", value);

    AddHeader(name, tmp);
}

bool HttpMessage::RemoveHeader(const char *name) {
    bool ret{false};

    for (size_t i = 0; header_name_list_.size() > i && ret == false; ++i) {
        if (strcasecmp(name, header_name_List_[i].c_str()) == 0) {
            header_name_list_.erase(header_name_list_.begin() + i);
            header_value_list_.erase(header_value_list_.begin() + i);
            ret = true;
        }
    }

    return ret;
}



}
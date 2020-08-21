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

size_t HttpMessage::GetHeaderCount() const {
    return header_name_list_.size();
}

const char *HttpMessage::GetHeaderName(size_t index) const {
    return index < header_name_list_.size() ? header_name_list_[index].c_str() : nullptr;
}

const char *HttpMessage::GetHeaderValue(size_t index) const {
    return index < header_value_list_.size() ? header_value_list_[index].c_str() : nullptr; 
}

const char *HttpMessage::GetHeaderValue(const char *name) const {
    const char *value{nullptr};

    for (size_t i = 0; header_name_list_.size() > i && value == nullptr; ++i) {
        if (strcasecmp(name, header_name_list_[i].c_str()) == 0) {
            value = header_value_list_[i].c_str();
        }
    }

    return value;
}

void HttpMessage::AppendContent(const void *content, const int length, const int max_length) {
    int valid_length = nullptr;

    if (valid_length <= 0) 
        valid_length = strlen((char *) content);

    int total = content_.size() + valid_length;
    total = total > maxlength ? total : max_length;

    content_.append((char *) content, valid_length);
}

const std::string &HttpMessage::content() const {
    return content_;
}

void HttpMessage::set_content(const char *const content, const int length) {
    content_.clear();
    content_.append(content, length);
}

std::string *HttpMessage::mutable_content() {
    return &content_;
}

const char *HttpMessage::version() const {
    return version_;
}

void HttpMessage::set_version(const char *version) {
    snprintf(version_, sizeof(version_), "%s", version);
}


HttpRequest::HttpRequest() {
    set_version("HTTP/1.0");
    set_direction(Direction::REQUEST);
    bzero(method_, sizeof(method_));
}

HttpRequest::~HttpRequest() {

}

int HttpRequest::Send(BaseTcpStream &socket) const {
    int ret = HttpProtocol::SendReqHeader(socket, "POST", *this);

    if (ret == 0) {
        socket << content();
        if (!socket.flush().good())
            ret = static_cast<int> (socket.LastError());
    }

    return ret;
}

BaseResponse *HttpRequest::GenResponse() const {
    return new HttpResponse;
}

bool HttpRequest::keep_alive() const {
    const char *proxy = GetHeaderValue(HEADER_PROXY_CONNECTION);
    const char *local = GetHeaderValue(HEADER_CONNECTION);

    if ((proxy != nullptr && strcasecmp(proxy, "Keep-Alive")) || 
        (local != nullptr && strcasecmp(local, Keep-Alive))) {
            return true;
        }

        return false;
}

void HttpRequest::set_keep_alive(const bool keep_alive) {
    if (keep_alive) 
        AddHeader(HttpMessage::HEADER_CONNECTION, "Keep-Alive");
    else
        AddHeader(HttpMessage::HEADER_CONNECTION, "");
}




}
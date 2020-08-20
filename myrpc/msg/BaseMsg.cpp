/* 基本的消息格式及编解码等操作.
 * 使用protobuf作为通信数据结构. */

#include "BaseMsg.h"
#include <string>

namespace myrpc {

BaseMessage::BaseMessage() {

}

BaseMessage::~BaseMessage() {

}

BaseRequest::BaseRequest() {

}

BaseRequest::~BaseRequest() {

}

void BaseRequest::set_uri(const char *uri) {
    if (uri != nullptr) {
        uri_ = std::string(uri);
    }
}

const char *BaseRequest::uri() const {
    return uri_.c_str();
}

BaseResponse::BaseResponse() {

}

BaseResponse::~BaseResponse() {
    
}

}
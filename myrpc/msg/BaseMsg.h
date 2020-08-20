/* 基本的消息格式及编解码等操作.
 * 使用protobuf作为通信数据结构. */

#pragma once 

#include "Common.h"
#include "../network.h"

//google
namespace google {
    //protobuf
    namespace protobuf {
        class Message;
    }
}

namespace myrpc {

class BaseMessage {
public:
    BaseMessage();
    virtual ~BaseMessage();

    virtual int Send(BaseTcpStream &socket) const = 0;
    virtual int ToPb(google::protobuf::Message *const message) const = 0;
    virtual int FromPb(const google::protobuf::Message &message) = 0;
    virtual size_t size() const = 0;

    bool fake() const { 
        return fake_;
    }

protected:
    void set_fake(const bool fake) {
        fake_ = fake;
    }

private:
    bool fake_{false};
};

class BaseResponse;

class BaseRequest : virtual public BaseMessage {
public:
    BaseRequest();
    virtual ~BaseRequest() override;

    virtual BaseResponse *GenResponse() const = 0;
    virtual bool keep_alive() const = 0;
    virtual void set_keep_alive(const bool keep_alive) = 0;

    void set_uri(const char *uri);
    const char *uri() const;

private:
    std::string  uri_;
};

class BaseResponse : virtual public BaseMessage {
public:
    enum class FakeReason {
        NONE = 0,
        DISPATCH_ERROR = 1
    };

    BaseResponse();
    virtual ~BaseResponse() override;

    virtual void SetFake(FakeReason reason) = 0;
    virtual  int Modify(const bool keep_alive, const std::string &version) = 0;

    virtual int result() = 0;
    virtual void set_result(const int result) = 0;
};

}
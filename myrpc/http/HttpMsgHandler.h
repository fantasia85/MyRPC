/* Http消息处理类，继承自BaseMessageHandler类. */

#pragma once 

#include "../msg/BaseMsgHandler.h"

namespace myrpc {

class HttpMessage;
class HttpRequest;
class HttpResponse;

class BaseTcpStream;

class HttpMessageHandler : public BaseMessageHandler {
public:
    HttpMessageHandler() = default;
    virtual ~HttpMessageHandler() override = default;

    virtual int RecvRequest(BaseTcpStream &socket, BaseRequest *&req) override;
    virtual int RecvResponse(BaseTcpStream &socket, BaseResponse *&resp) override;

    virtual int GenRequest(BaseRequest *&req) override;
    virtual int GenResponse(BaseResponse *&resp) override;

    virtual bool keep_alive() const override;

private:
    std::string version_;
    bool keep_alive_ = false;
};

}
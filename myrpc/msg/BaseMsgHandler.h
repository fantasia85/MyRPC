/* 基本消息的处理类，为抽象类. */

#pragma once 

#include "BaseMsg.h"
#include "../network.h"

namespace myrpc {

class BaseMessageHandler {
public:
    BaseMessageHandler() = default;
    virtual ~BaseMessageHandler() = default;

    virtual int RecvRequest(BaseTcpStream &socket, BaseRequest *&req) = 0;
    virtual int RecvResponse(BaseTcpStream &socket, BaseResponse *&resp) = 0;

    virtual int  GenRequest(BaseRequest *&req) = 0;
    virtual int GenResponse(BaseResponse *&resp) = 0;

    virtual bool keep_alive() const = 0;

protected:
    BaseRequest *req_ = nullptr;
};

}
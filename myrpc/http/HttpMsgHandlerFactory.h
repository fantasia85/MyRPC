/* Http消息处理类的工厂类，继承自BaseMessageHandlerFactory类. */

#pragma once 

#include "../msg.h"

namespace myrpc {

class HttpMessageHandlerFactory : virtual public BaseMessageHandlerFactory {
public:
    HttpMessageHandlerFactory() = default;
    virtual ~HttpMessageHandlerFactory() override = default;

    virtual std::unique_ptr<BaseMessageHandler> Create() override;
};

}

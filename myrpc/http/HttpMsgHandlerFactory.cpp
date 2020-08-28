/* Http消息处理类的工厂类，继承自BaseMessageHandlerFactory类. */

#include "HttpMsgHandlerFactory.h"
#include "HttpMsgHandler.h"
#include <memory>

namespace myrpc {

std::unique_ptr<BaseMessageHandler> HttpMessageHandlerFactory::Create() {
    return std::move(std::unique_ptr<BaseMessageHandler> (new HttpMessageHandler));
}

}
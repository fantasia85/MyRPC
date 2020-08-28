/* 基本消息处理类的工厂类，为抽象类. */

#pragma once 

#include <functional>
#include <memory>

namespace myrpc {

class BaseMessageHandlerFactory;

using BaseMessageHandlerFactoryCreateFunc = std::function<std::unique_ptr<BaseMessageHandlerFactory> ()>;

class BaseMessageHandler;

class BaseMessageHandlerFactory {
public:
    BaseMessageHandlerFactory() = default;
    virtual ~BaseMessageHandlerFactory() = default;

    virtual  std::unique_ptr<BaseMessageHandler> Create() = 0;
};

}
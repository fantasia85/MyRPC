/* 定义了协程接口的基类.
 * 定义了一个静态函数对象，用来创建协程的上下文.
 * */ 

#pragma once 

#include <unistd.h>
#include <functional>

namespace myrpc {

class UThreadContext;

typedef std::function<void (void *)> UThreadFunc_t;
typedef std::function<void()> UThreadDoneCallback_t;
typedef std::function<UThreadContext *(size_t, UThreadFunc_t, void *,
    UThreadDoneCallback_t, const bool)> ContextCreateFunc_t;

class UThreadContext {
public:
    UThreadContext() { }
    virtual ~UThreadContext() { }

    static UThreadContext *Create(size_t stack_size, UThreadFunc_t func, void *args,
        UThreadDoneCallback_t callback, const bool need_stack_protect); 

    static void SetContextCreateFunc(ContextCreateFunc_t context_create_func);
    static ContextCreateFunc_t GetContextCreateFunc();

    virtual void Make(UThreadFunc_t func, void *args) = 0;
    virtual bool Resume() = 0;
    virtual bool Yield() = 0;
    
private: 
    static ContextCreateFunc_t context_create_func_;
};

}
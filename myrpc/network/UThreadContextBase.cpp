/* 定义了协程接口的基类.
 * 定义了一个静态函数对象，用来创建协程的上下文.
 * */ 

#include "UThreadContextBase.h"

namespace myrpc {

ContextCreateFunc_t UThreadContext::context_create_func_ = nullptr;

UThreadContext *UThreadContext::Create(size_t stack_size, UThreadFunc_t func, void *args,
    UThreadDoneCallback_t callback, const bool need_stack_protect) {
        if (context_create_func_ != nullptr) 
            return context_create_func_(stack_size, func, args, callback, need_stack_protect);
        return nullptr;    
}

void UThreadContext::SetContextCreateFunc(ContextCreateFunc_t context_create_func) {
    context_create_func_ = context_create_func;
}

ContextCreateFunc_t UThreadContext::GetContextCreateFunc() {
    return context_create_func_;
}

}
/* 继承了UThreadContext，默认使用ucontext作实现的协程上下文.
 * */ 

#pragma once

#include "UThreadContextBase.h"
#include "UThreadContextUtil.h"
#include <ucontext.h>

namespace myrpc {

class UThreadContextSystem : public UThreadContext {
public:
    UThreadContextSystem(size_t stack_size, UThreadFunc_t func, void *args, 
        UThreadDoneCallback_t callback, const bool need_stack_protect);
    ~UThreadContextSystem();

    static UThreadContext *DoCreate(size_t stack_size, UThreadFunc_t func, void *args,
        UThreadDoneCallback_t callback, const bool need_stack_protect);
    
    //创建一个协程上下文
    void Make(UThreadFunc_t func, void *args) override; 
    //切换到一个协程
    bool Resume() override;
    //切出当前协程
    bool Yield() override;

    ucontext_t *GetMainContext();

private:
    //包装了协程的执行函数，协程运行时会切换到这个函数
    static void UThreadFuncWrapper(uint32_t low32, uint32_t high32);

    ucontext_t context_;
    UThreadFunc_t func_;
    void *args_;
    UThreadStackMemory stack_;
    UThreadDoneCallback_t callback_;
};

}
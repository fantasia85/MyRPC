/* 继承了UThreadContext，默认使用ucontext作实现的协程上下文.
 * */ 

#include "UThreadContextSystem.h"
#include <unistd.h>

namespace myrpc {

UThreadContextSystem::UThreadContextSystem(size_t stack_size, UThreadFunc_t func, void *args, 
    UThreadDoneCallback_t callback, const bool need_stack_protect) 
    : func_(func),  args_(args), stack_(stack_size, need_stack_protect), callback_(callback) {
        Make(func, args);
}

UThreadContextSystem::~UThreadContextSystem() {

}

UThreadContext *UThreadContextSystem::DoCreate(size_t stack_size, UThreadFunc_t func, void *args,
    UThreadDoneCallback_t callback, const bool need_stack_protect) {
        return new UThreadContextSystem(stack_size, func, args, callback, need_stack_protect);
}

//创建一个协程上下文
void UThreadContextSystem::Make(UThreadFunc_t func, void *args) {
    func_ = func;
    args_ = args;
    getcontext(&context_);
    context_.uc_stack.ss_sp = stack_.top();
    context_.uc_stack.ss_size = stack_.size();
    context_.uc_stack.ss_flags = 0;
    context_.uc_link = GetMainContext();
    uintptr_t ptr = (uintptr_t) this;
    makecontext(&context_, (void (*)(void))UThreadContextSystem::UThreadFuncWrapper, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
}

//切换到一个协程
bool UThreadContextSystem::Resume() {
    swapcontext(GetMainContext(), &context_);
    return true;
}

//切出当前协程
bool UThreadContextSystem::Yield() {
    swapcontext(&context_, GetMainContext());
    return true;
}

ucontext_t *UThreadContextSystem::GetMainContext() {
    //每个线程一个的main_context，每次调用Yield()会切换到该协程
    static __thread ucontext_t main_context;
    return &main_context;
}

//包装了协程的执行函数，协程运行时会切换到这个函数
void UThreadContextSystem::UThreadFuncWrapper(uint32_t low32, uint32_t high32) {
    uintptr_t ptr = (uintptr_t) low32 | ((uintptr_t) high32 << 32);
    UThreadContextSystem *uc = (UThreadContextSystem *) ptr;
    uc->func_(uc->args_);
    if (uc->callback_ != nullptr) {
        uc->callback_();
    }
}

}
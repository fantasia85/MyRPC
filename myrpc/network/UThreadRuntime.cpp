/* 封装了协程的调度，并用一个context_list_保存所有协程上下文 ，
 * 每一个上下文被封装成一个ContextSlot.
 * */

#include "UThreadRuntime.h"
#include "UThreadContextSystem.h"
#include <unistd.h>
#include <assert.h>

//协程的当前状态,status
enum {
    UTHREAD_RUNNING,
    UTHREAD_SUSPEND,
    UTHREAD_DONE
};

namespace myrpc {

UThreadRuntime::UThreadRuntime(size_t stack_size, const bool need_stack_protect)
    : stack_size_(stack_size), first_done_item_(-1), current_uthread_(-1), 
        unfinished_item_count_(0), need_stack_protect_(need_stack_protect) {
    if (UThreadContext::GetContextCreateFunc() == nullptr)
        UThreadContext::SetContextCreateFunc(UThreadContextSystem::DoCreate);
}

UThreadRuntime::~UThreadRuntime() {
    for (auto &context_slot : context_list_) {
        delete context_slot.context;
    }
}

//创建一个上下文
int UThreadRuntime::Create(UThreadFunc_t func, void *args) {
    if (func == nullptr)
        return  -2;
    
    int index = -1;
    //first_done_iterm_大于0，说明此时有已经执行完的协程，更新first_done_item_的值
    //然后直接更换此协程的上下文
    if (first_done_item_ >= 0) {
        index = first_done_item_;
        first_done_item_ = context_list_[index].next_done_item;
        context_list_[index].context->Make(func, args);
    }
    else {
        //若当前没有已执行完的协程，则在ContextSlot中添加一个Slot
        index = context_list_.size();
        auto new_context = UThreadContext::Create(stack_size_, func, args,
            std::bind(&UThreadRuntime::UThreadDoneCallback, this), need_stack_protect_);
        assert(new_context != nullptr);
        ContextSlot context_slot;
        context_slot.context = new_context;
        context_list_.push_back(context_slot);
    }

    context_list_[index].next_done_item = -1;
    context_list_[index].status = UTHREAD_SUSPEND;
    unfinished_item_count_++;
    return index;
}

void UThreadRuntime::UThreadDoneCallback() {
    if (current_uthread_ != -1) {
        ContextSlot &context_slot = context_list_[current_uthread_];
        //将当前的first_done_item_保存到next_done_item，并更新自身
        context_slot.next_done_item = first_done_item_;
        context_slot.status = UTHREAD_DONE;
        first_done_item_ = current_uthread_;
        unfinished_item_count_--;
        current_uthread_ = -1;
    }
}

//封装了UThreadContext的对应操作，切换到一个协程
bool UThreadRuntime::Resume(size_t index) {
    if (index >= context_list_.size())
        return false;

    auto context_slot = context_list_[index];
    if (context_slot.status == UTHREAD_SUSPEND) {
        current_uthread_ = index;
        context_slot.status = UTHREAD_RUNNING;
        context_slot.context->Resume();
        return true;
    }
    return false;
}

//封装了UThreadContext的对应操作，切出当前协程
bool UThreadRuntime::Yield() {
    if (current_uthread_ != -1) {
        auto context_slot = context_list_[current_uthread_];
        current_uthread_ = -1;
        context_slot.status = UTHREAD_SUSPEND;
        context_slot.context->Yield();
    }

    return true;
}

int UThreadRuntime::GetCurrUThread() {
    return current_uthread_;
}

bool UThreadRuntime::IsAllDone() {
    return unfinished_item_count_ == 0;
}

int UThreadRuntime::GetUnfinishedItemCount() const {
    return unfinished_item_count_;
}

}
/* 封装了协程的调度，并用一个context_list_保存所有协程上下文 ，
 * 每一个上下文被封装成一个ContextSlot.
 * */

#pragma once 

#include "UThreadContextBase.h"
#include <vector>

namespace myrpc {

class UThreadRuntime  {
public:
    UThreadRuntime(size_t stack_size, const bool need_stack_protect);
    ~UThreadRuntime();

    //创建一个上下文
    int Create(UThreadFunc_t func, void *args);
    int GetCurrUThread();
    //封装了UThreadContext的对应操作
    bool Yield();
    //封装了UThreadContext的对应操作
    bool Resume(size_t index);
    bool IsAllDone();
    int GetUnfinishedItemCount() const;

    void UThreadDoneCallback();

private:
    //把每个上下文封装成一个ContextSlot的结构
    struct ContextSlot {
        ContextSlot() {
            context = nullptr;
            next_done_item = -1;
        }
        UThreadContext  *context;
        //保存下一个可用的Slot下标
        int next_done_item;
        int status;
    };

    size_t stack_size_;
    std::vector<ContextSlot> context_list_;
    //保存上一个已经完成的上下文下标
    int first_done_item_;
    int current_uthread_;
    int unfinished_item_count_;
    bool need_stack_protect_;
};

}
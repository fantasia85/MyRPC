/* 定义了UThreadStackMemory，是每个协程的私有栈.
 * 并没有实现共享栈，不使用malloc，而是用mmap.
 * 设置标志变量来选择是否开启保护模式.
 * 调用mmap时设置内存段为匿名的，不需要读写fd，
 * 并且为私有映射，不与其他进程共享. 
 * */

#include "UThreadContextUtil.h"
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>

namespace myrpc {

UThreadStackMemory::UThreadStackMemory(const size_t stack_size, const bool need_protect)
    : raw_stack_(nullptr), stack_(nullptr), need_protect_(need_protect) {
    
    //获得每一页的大小
    int page_size = getpagesize();

    //不为整页时将其扩充到整页的大小
    if ((stack_size % page_size) != 0) {
        stack_size_ = (stack_size / page_size + 1) * page_size;
    }
    else 
        stack_size_ = stack_size;

    if (need_protect) {
        //设置为匿名的和私有映射，不与其他进程共享
        raw_stack_ = mmap(NULL, stack_size_ + page_size * 2, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        assert (raw_stack_ != nullptr);

        //设置为禁止访问
        mprotect(raw_stack_, page_size, PROT_NONE);
        mprotect((void *) ((char *) raw_stack_ + stack_size_ + page_size), page_size, PROT_NONE);

        stack_ = (void *)((char *) raw_stack_ + page_size);
    }
    else {
        raw_stack_ = mmap(NULL, stack_size_, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        assert(raw_stack_ != nullptr);
        stack_ = raw_stack_;
    }
}

UThreadStackMemory::~UThreadStackMemory() {
    int page_size = getpagesize();
    if (need_protect_) {
        mprotect(raw_stack_, page_size, PROT_READ | PROT_WRITE);
        mprotect((void *) ((char *) raw_stack_ + stack_size_ + page_size), page_size, PROT_READ | PROT_WRITE);
        munmap(raw_stack_, stack_size_ + page_size * 2);
    }
    else 
        munmap(raw_stack_, stack_size_);
}

void *UThreadStackMemory::top() {
    return stack_;
}

size_t UThreadStackMemory::size() {
    return stack_size_;
}

}
/* 定义了UThreadStackMemory，是每个协程的私有栈.
 * 并没有实现共享栈，不使用malloc，而是用mmap.
 * 设置标志变量来选择是否开启保护模式.
 * 调用mmap时设置内存段为匿名的，不需要读写fd，
 * 并且为私有映射，不与其他进程共享. 
 * */

#pragma once 

#include <memory>
#include <sys/mman.h>

namespace myrpc {

class UThreadStackMemory {
public:
    UThreadStackMemory(const size_t stack_size, const bool need_protect = true);
    ~UThreadStackMemory();

    void *top();
    size_t size();

private:
    //协程私有栈整体的起始位置
    void *raw_stack_;
    //协程私有栈可写入的起始位置
    void *stack_;
    //协程私有栈可写入的大小
    size_t stack_size_;
    //是否开启保护模式
    int need_protect_;
};

}
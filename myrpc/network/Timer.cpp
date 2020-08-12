/* 使用小根堆来管理超时，使用vector来管理堆，
 * 并设计成可以在任何合法的位置删除节点 
 * */

#include "Timer.h"
#include "UThreadEpoll.h"
#include <chrono>
#include <ctime>
#include <errno.h>

namespace myrpc {

//获得系统时间，可以被用户设置
const uint64_t Timer::GetTimestampMS() {
    auto now_time = std::chrono::system_clock::now();
    uint64_t now = (std::chrono::duration_cast<std::chrono::milliseconds>(now_time.time_since_epoch())).count();
    return now;
}

//获得monotonic时间，不可以被用户随意设置
const uint64_t Timer::GetSteadyClockMS() {
    auto  now_time = std::chrono::steady_clock::now();
    uint64_t  now = (std::chrono::duration_cast<std::chrono::milliseconds>(now_time.time_since_epoch())).count();
    return now;
}

void Timer::MsSleep(const int time_ms) {
    timespec  t;
    t.tv_sec = time_ms / 1000;
    t.tv_nsec = (time_ms) % 1000 * 1000000;
    int ret = 0;
    do {
        ret = ::nanosleep(&t, &t);
    } while (ret == -1 && errno == EINTR);
}

Timer::Timer() { 

}

Timer::~Timer() {

}

//向上调整堆
void Timer::heap_up(const size_t end_idx) {
    size_t now_idx = end_idx - 1;
    TimerObj obj = timer_heap_[now_idx];
    size_t parent_idx = (now_idx - 1) / 2;
    while (now_idx > 0 && parent_idx >= 0 && obj < timer_heap_[parent_idx]) {
        timer_heap_[now_idx] = timer_heap_[parent_idx];
        UThreadSocketSetTimerID(*timer_heap_[now_idx].socket_, now_idx + 1);
        now_idx = parent_idx;
        parent_idx = (now_idx - 1) / 2;
    }

    timer_heap_[now_idx] = obj; 
    UThreadSocketSetTimerID(*timer_heap_[now_idx].socket_, now_idx + 1);
}

//向下调整堆
void Timer::heap_down(const size_t begin_idx) {
    size_t now_idx = begin_idx;
    TimerObj obj = timer_heap_[now_idx];
    size_t child_idx = (now_idx + 1) * 2;
    while (child_idx <= timer_heap_.size()) {
        if (child_idx == timer_heap_.size() || 
            timer_heap_[child_idx - 1] < timer_heap_[child_idx]) {
                child_idx--;
        }

        if (obj < timer_heap_[child_idx] || obj == timer_heap_[child_idx]) {
            break;
        }

        timer_heap_[now_idx] = timer_heap_[child_idx];
        UThreadSocketSetTimerID(*timer_heap_[now_idx].socket_, now_idx + 1);
        now_idx = child_idx;
        child_idx = (now_idx + 1) * 2;   
    }

    timer_heap_[now_idx] = obj;
    UThreadSocketSetTimerID(*timer_heap_[now_idx].socket_, now_idx + 1);
}

}
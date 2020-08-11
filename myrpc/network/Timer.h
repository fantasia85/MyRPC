/* 使用小根堆来管理超时，使用vector来管理堆，
 * 并设计成可以在任何合法的位置删除节点 
 * */

#pragma once 

#include <cstdlib>
#include <vector>
#include <cinttypes>

namespace myrpc {

typedef struct tagUThreadSocket UThreadSocket_t;

class Timer final {
public:
    Timer();
    ~Timer();

    void AddTimer(uint64_t abs_time, UThreadSocket_t *socket);
    void RemoveTimer(const size_t timer_id);
    UThreadSocket_t *PopTimeout();
    const int GetNextTimeout() const;
    const bool empty();
    static const uint64_t GetTimestampMS();
    static const uint64_t GetSteadyClockMS();
    static void MsSleep(const int time_ms);
    std::vector<UThreadSocket_t *> GetSocketList();

private:
    void heap_up(const size_t end_idx);
    void heap_down(const size_t begin_idx);

    struct TimerObj {
        TimerObj(uint64_t abs_time, UThreadSocket_t *socket) 
            : abs_time_(abs_time), socket_(socket) {

        }

        uint64_t abs_time_;
        UThreadSocket_t *socket_;

        bool operator <(const TimerObj &obj) const {
            return abs_time_ < obj.abs_time_;
        }

        bool operator ==(const TimerObj &obj) const {
            return abs_time_ == obj.abs_time_;
        }
    };

    std::vector<TimerObj> timer_heap_;
};

}
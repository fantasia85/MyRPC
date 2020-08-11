/* 封装了epoll驱动的协程调度，并封装了socket及其他相关资源.
 * 协程的调度由UThreadEpollScheduler来操作.
 * */

#pragma once

#include "Timer.h"
#include "UThreadRuntime.h"
#include <functional>

namespace myrpc {

class UThreadEpollScheduler;

typedef struct tagUThreadSocket UThreadSocket_t;

typedef std::pair<UThreadEpollScheduler *, int> UThreadEpollArgs_t;

typedef std::function<UThreadSocket_t *()> UThreadActiveSocket_t;
typedef std::function<void()> UThreadHandlerAcceptedFdFunc_t;
typedef std::function<void()> UThreadHandlerNewRequest_t;

class EpollNotifier final {


};

}
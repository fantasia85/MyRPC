/* 封装了epoll驱动的协程调度，并封装了socket及其他相关资源.
 * 协程的调度由UThreadEpollScheduler来操作.
 * */

#include "UThreadEpoll.h"
#include <sys/epoll.h>

namespace myrpc {

typedef struct tagUThreadSocket {
    UThreadEpollScheduler *scheduler;
    int uthread_id;

    int epoll_fd;

    int socket;
    int connect_timeout_ms;
    int socket_timeout_ms;

    int waited_events;
    //保存该socket的定时器在heap数组中的位置，如果存在，方便进行查找
    size_t timer_id;
    struct epoll_event event;
    void *args;
} UThreadSocket_t;   

}
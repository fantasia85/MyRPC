/* 封装了epoll驱动的协程调度，并封装了socket及其他相关资源.
 * 协程的调度由UThreadEpollScheduler来操作.
 * */

#include "UThreadEpoll.h"
#include "SocketStreamBase.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iterator>
#include <cassert>

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

EpollNotifier::EpollNotifier(UThreadEpollScheduler *scheduler)
    : scheduler_(scheduler) {
        pipe_fds_[0] = pipe_fds_[1] = -1;
}

EpollNotifier::~EpollNotifier() {
    if (pipe_fds_[0] != -1) 
        close(pipe_fds_[0]);
    if (pipe_fds_[1] != - 1)
        close(pipe_fds_[1]);
}

//将Func函数加入调度器任务队列
void EpollNotifier::Run() {
    //创建管道
    pipe(pipe_fds_);
    //assert
    fcntl(pipe_fds_[1], F_SETFL, O_NONBLOCK);
    scheduler_->AddTask(std::bind(&EpollNotifier::Func, this), nullptr);    
}

//读取管道，唤醒epoll
void EpollNotifier::Func() {
    UThreadSocket_t *socket{scheduler_->CreateSocket(pipe_fds_[0], -1, -1, false)};
    char tmp[2] = {0};
    while (true) {
        if (UThreadRead(*socket, tmp, 1, 0) < 0) 
            break;
    }
    free(socket);
}

void EpollNotifier::Notify() {
    ssize_t write_len = write(pipe_fds_[1], (void *)"a", 1);
    if (write_len < 0) {
        //log
    }
}


UThreadNotifier::UThreadNotifier() {
    pipe_fds_[0] = pipe_fds_[1] = -1;
}

UThreadNotifier::~UThreadNotifier() {
    free(socket_);
    if (pipe_fds_[0] != -1) 
        close(pipe_fds_[0]);
    if (pipe_fds_[1] != -1)
        close(pipe_fds_[1]);
}

int UThreadNotifier::Init(UThreadEpollScheduler *const scheduler,
        const int timeout_ms) {
    int ret {pipe(pipe_fds_)};

    if (ret != 0)
        return ret;

    fcntl(pipe_fds_[1], F_SETFL, O_NONBLOCK);
    socket_ = scheduler->CreateSocket(pipe_fds_[0], timeout_ms, -1, false);

    return 0;
}

int UThreadNotifier::SendNotify(void *const data) {
    data_ = data;
    ssize_t write_len {write(pipe_fds_[1], (void *) "a", 1)};

    if (write_len < 0) {
        //log
        return -1;
    }

    return 0;
}

int UThreadNotifier::WaitNotify(void *&data) {
    char buf;
    ssize_t read_len{UThreadRead(*socket_, (void *) &buf, 1, 0)};

    if (read_len < 0) {
        //log
        return -1;
    }

    data = data_;
    data_ = nullptr;

    return 0;
}

UThreadNotifierPool::UThreadNotifierPool(UThreadEpollScheduler *const scheduler, 
    const int timeout_ms) : scheduler_(scheduler), timeout_ms_(timeout_ms) {

}

UThreadNotifierPool::~UThreadNotifierPool() {
    for (auto &it : pool_map_) 
        delete it.second;
}

int UThreadNotifierPool::GetNotifier(const std::string &id, UThreadNotifier *&notifier) {
    notifier = nullptr;

    auto it (pool_map_.find(id));
    if (pool_map_.end() != it && it->second) {
        notifier = it->second;
        return 0;
    }

    notifier = new UThreadNotifier();
    int ret {notifier->Init(scheduler_, timeout_ms_)};

    if (ret != 0) 
        return ret;

    pool_map_[id] = notifier;

    return 0;
}

void UThreadNotifierPool::ReleaseNotifier(const std::string &id) {
    if (pool_map_[id]) {
        delete pool_map_[id];
        pool_map_[id] = nullptr;
    }
    pool_map_.erase(id);
}

enum UThreadEpollREventStatus {
    UThreadEpollREvent_Timeout = 0,
    UThreadEpollREvent_Error = -1,
    UThreadEpollREvent_Close = -2,
};

UThreadEpollScheduler::UThreadEpollScheduler(size_t stack_size, int max_task, 
    const bool need_stack_protect) 
    : runtime_(stack_size, need_stack_protect), epoll_wake_up_(this) {
    max_task_ = max_task + 1;

    epoll_fd_ = epoll_create(max_task_);

    if (epoll_fd_ < 0) {
        //log
        assert(epoll_fd_ >= 0);
    }

    closed_ = false;
    run_forever_ = false;
    active_socket_func_ = nullptr;
    handler_accepted_fd_func_ = nullptr;
    handler_new_request_func_ = nullptr;

    epoll_wait_events_ = 0;
    epoll_wait_events_per_second_ = 0;
    epoll_wait_events_last_cal_time_ = Timer::GetSteadyClockMS();
}

UThreadEpollScheduler::~UThreadEpollScheduler() {
    close(epoll_fd_);
}

UThreadEpollScheduler *UThreadEpollScheduler::Instance() {
    static UThreadEpollScheduler obj(64 * 1024, 300);
    return &obj;
}

bool UThreadEpollScheduler::IsTaskFull() {
    return (runtime_.GetUnfinishedItemCount() + (int)todo_list_.size()) >= max_task_;
}



}
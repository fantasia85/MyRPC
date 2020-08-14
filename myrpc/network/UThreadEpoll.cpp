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
#include <limits>

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

//将任务添加到todo_list_中
void UThreadEpollScheduler::AddTask(UThreadFunc_t func, void *args) {
    todo_list_.push(std::make_pair(func, args));
}

void UThreadEpollScheduler::SetActiveSocketFunc(UThreadActiveSocket_t active_socket_func) {
    active_socket_func_ = active_socket_func;
}

void UThreadEpollScheduler::SetHandlerNewRequestFunc(UThreadHandlerNewRequest_t handler_new_request_func) {
    handler_new_request_func_ = handler_new_request_func;
}

void UThreadEpollScheduler::SetHandlerAcceptedFdFunc(UThreadHandlerAcceptedFdFunc_t handler_accepted_fd_func) {
    handler_accepted_fd_func_ = handler_accepted_fd_func;
}

bool UThreadEpollScheduler::YieldTask() {
    return  runtime_.Yield();
}

int UThreadEpollScheduler::GetCurrUThread() {
    return runtime_.GetCurrUThread();
}

UThreadSocket_t *UThreadEpollScheduler::CreateSocket(const int fd, 
    const int socket_timeout_ms, const int connect_timeout_ms, const bool no_delay) {
    UThreadSocket_t  *socket = (UThreadSocket_t *) calloc (1, sizeof(UThreadSocket_t));

    BaseTcpUtils::SetNonBlock(fd, true);
    if (no_delay) 
        BaseTcpUtils::SetNoDelay(fd, true);

    socket->scheduler = this;
    socket->epoll_fd = epoll_fd_;
    socket->event.data.ptr = socket;

    socket->socket = fd;
    socket->connect_timeout_ms = connect_timeout_ms;
    socket->socket_timeout_ms = socket_timeout_ms;

    socket->waited_events = 0;
    socket->args = nullptr;

    return socket;
}

/* 将任务队列中的函数创建为协程，并Resume切换到该协程，
 * 协程中会将fd相应的操作在epoll中注册然后Yield回到Run */
void UThreadEpollScheduler::ConsumeTodoList() {
    while (!todo_list_.empty()) {
        auto &it = todo_list_.front();
        int id = runtime_.Create(it.first, it.second);
        runtime_.Resume(id);

        todo_list_.pop();
    }
}

void UThreadEpollScheduler::Close() {
    closed_ = true;
}

void UThreadEpollScheduler::NotifyEpoll() {
    if (epoll_wait_events_per_second_ < 2000) {
        //log
        epoll_wake_up_.Notify();
    }
}

void UThreadEpollScheduler::ResumeAll(int flag) {
    std::vector<UThreadSocket_t *> exist_socket_list = timer_.GetSocketList();
    for (auto &socket : exist_socket_list) {
        socket->waited_events = flag;
        runtime_.Resume(socket->uthread_id);
    }
}

/* 首先调用EpollNotifier的Run函数，将Func函数加入调度器的任务队列，Func函数会读取管道，唤醒epoll.
 * 下一步会将任务队列中的函数创建为协程，并Resume切换到协程，协程中会将fd的相应操作在epoll中注册，
 * 然后Yield回到回到Run. Run函数检查活动的fd，并Resume到活动的协程中进行IO操作。 */
void UThreadEpollScheduler::RunForever() {
    run_forever_ = true;
    epoll_wake_up_.Run();
    Run();
}

void UThreadEpollScheduler::StatEpollwaitEvents(const int event_count) {
    epoll_wait_events_ += event_count;
    auto now_time = Timer::GetSteadyClockMS();
    if (now_time > epoll_wait_events_last_cal_time_ + 1000) {
        epoll_wait_events_per_second_ = epoll_wait_events_;
        epoll_wait_events_ = 0;
        epoll_wait_events_last_cal_time_ = now_time;
        //log
    }
}

/* Run函数先会将任务队列中的函数创建为协程，并Resume切换到协程，协程中会将fd的相应操作在epoll中注册，
 * 然后Yield回到回到Run. Run函数检查活动的fd，并Resume到活动的协程中进行IO操作. */
bool UThreadEpollScheduler::Run() {
    //将任务队列中的函数创建为协程
    ConsumeTodoList();

    struct epoll_event *events = (struct epoll_event *) calloc (max_task_, sizeof(struct epoll_event));

    int next_timeout = timer_.GetNextTimeout();

    for ( ; (run_forever_) || (!runtime_.IsAllDone()); ) {
        /* 监听fd上的活动事件 ，并把超时事件设置为4 ms.
         * 如果超时，则轮询的处理工作. */
        int nfds = epoll_wait(epoll_fd_, events, max_task_, 4);
        if (nfds != -1) {
            //处理socket上的活动事件
            for (int i = 0; i < nfds; i++) {
                UThreadSocket_t *socket = (UThreadSocket_t *) events[i].data.ptr;
                socket->waited_events = events[i].events;

                runtime_.Resume(socket->uthread_id);
            }

            //轮询工作
            //读取data flow，如果有responce可以读取，通知相应的socket写给client
            if (active_socket_func_ != nullptr) {
                UThreadSocket_t *socket = nullptr;
                while ((socket = active_socket_func_()) != nullptr)
                    runtime_.Resume(socket->uthread_id);
            }

            //处理新的request
            if (handler_new_request_func_ != nullptr) 
                handler_new_request_func_();
            
            //处理新接收的client
            if (handler_accepted_fd_func_ != nullptr) 
                handler_accepted_fd_func_();

            if (closed_) {
                ResumeAll(UThreadEpollREvent_Close);
                break;
            }

            ConsumeTodoList();

            //处理超时事件
            DealwithTimeout(next_timeout);
        }
        else if (errno != EINTR)  {
            ResumeAll(UThreadEpollREvent_Error);
            break;
        }

        StatEpollwaitEvents(nfds);
    }
    
    free(events);

    return true;
}

void UThreadEpollScheduler::AddTimer(UThreadSocket_t *socket, const int timeout_ms) {
    RemoveTimer(socket->timer_id);

    if (timeout_ms == -1) {
        timer_.AddTimer((std::numeric_limits<uint64_t>::max)(), socket);
    }
    else {
        //先获得当前的时间，再在该时间上加上timeout_ms
        timer_.AddTimer(Timer::GetSteadyClockMS() + timeout_ms, socket);
    }
}

void UThreadEpollScheduler::RemoveTimer(const size_t timer_id) {
    if (timer_id > 0)
        timer_.RemoveTimer(timer_id);
}

//处理超时事件
void UThreadEpollScheduler::DealwithTimeout(int &next_timeout) {
    while (true) {
        next_timeout = timer_.GetNextTimeout();
        if (0 != next_timeout) {
            break;
        }

        UThreadSocket_t *socket = timer_.PopTimeout();
        socket->waited_events = UThreadEpollREvent_Timeout;
        runtime_.Resume(socket->uthread_id);
    }
}


//接受一个socket的版本 
int UThreadPoll(UThreadSocket_t &socket, int events, int *revents, const int timeout_ms) {
    int ret{-1};

    //获得当前正在执行的协程
    socket.uthread_id = socket.scheduler->GetCurrUThread();

    //增加超时事件
    socket.event.events = events;

    socket.scheduler->AddTimer(&socket, timeout_ms);
    //事件加入epoll中进行调度，为了处理当超时事件还没达到时就触发事件，使得提前结束
    epoll_ctl(socket.epoll_fd, EPOLL_CTL_ADD, socket.socket, &socket.event);

    //将当前的协程停止，转让CPU给主协程
    //当主协程下一次收到这个超时事件的时候，将执行权还给这个协程
    //协程从当前位置开始执行
    socket.scheduler->YieldTask();

    //超时任务完成，协程继续从此处执行，并删除定时器 
    epoll_ctl(socket.epoll_fd, EPOLL_CTL_DEL, socket.socket, &socket.event);
    socket.scheduler->RemoveTimer(socket.timer_id);

    *revents = socket.waited_events;

    if ((*revents) > 0) {
        if ((*revents) & events) 
            ret = 1;
        else {
            errno = EINVAL;
            ret = 0;
        }
    }
    else if ((*revents) == UThreadEpollREvent_Timeout) {
        //timeout
        errno = ETIMEDOUT;
        ret = 0;
    }
    else if ((*revents) == UThreadEpollREvent_Error) {
        //error
        errno = ECONNREFUSED;
        ret = -1;
    }
    else {
        //active close
        errno = 0;
        ret = -1;
    }

    return -1;
}

//接受一组socket的版本 
int UThreadPoll(UThreadSocket_t *list[], int count, const int timeout_ms) {
    int nfds = -1;

    UThreadSocket_t *socket = list[0];

    int epollfd = epoll_create(count);

    for (int i = 0; i < count; i++)
        epoll_ctl(epollfd, EPOLL_CTL_ADD, list[i]->socket, &(list[i]->event));

    UThreadSocket_t fake_socket;
    fake_socket.scheduler = socket->scheduler;
    fake_socket.socket = epollfd;
    fake_socket.uthread_id = socket->scheduler->GetCurrUThread();
    fake_socket.event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    fake_socket.event.data.ptr = &fake_socket;
    fake_socket.waited_events = 0;

    epoll_ctl(socket->epoll_fd, EPOLL_CTL_ADD, epollfd, &(fake_socket.event));

    socket->scheduler->YieldTask();

    if (0 != (EPOLLIN & fake_socket.waited_events)) {
        struct epoll_event *events = (struct epoll_event *) calloc (count, sizeof(struct epoll_event));

        nfds = epoll_wait(epollfd, events, count, 0);

        for (int i = 0; i < nfds; i++) {
            UThreadSocket_t *socket = (UThreadSocket_t *) events[i].data.ptr;
            socket->waited_events = events[i].events;
        }
    }
    else if (fake_socket.waited_events == 0) 
        nfds = 0;

    close (epollfd);

    return nfds;
}

}
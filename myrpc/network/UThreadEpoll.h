/* 封装了epoll驱动的协程调度，并封装了socket及其他相关资源.
 * 协程的调度由UThreadEpollScheduler来操作.
 * */

#pragma once

#include "Timer.h"
#include "UThreadRuntime.h"
#include <functional>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <arpa/inet.h>

namespace myrpc {

//封装了协程的调度
class UThreadEpollScheduler;

typedef struct tagUThreadSocket UThreadSocket_t;

typedef std::pair<UThreadEpollScheduler *, int> UThreadEpollArgs_t;

typedef std::function<UThreadSocket_t *()> UThreadActiveSocket_t;
typedef std::function<void()> UThreadHandlerAcceptedFdFunc_t;
typedef std::function<void()> UThreadHandlerNewRequest_t;

class EpollNotifier final {
public:
    EpollNotifier(UThreadEpollScheduler *scheduler);
    ~EpollNotifier();

    void Run();
    void Func();
    void Notify();

private:
    UThreadEpollScheduler  *scheduler_{nullptr};
    int pipe_fds_[2];
};

class UThreadNotifier final {
public:
    UThreadNotifier();
    ~UThreadNotifier();

    int Init(UThreadEpollScheduler *const scheduler, const int timeout_ms);
    int SendNotify(void *const data);
    int WaitNotify(void *&data);

private:
    UThreadEpollScheduler *scheduler_ {nullptr};
    UThreadSocket_t *socket_{nullptr};
    int pipe_fds_[2];
    void *data_{nullptr};
};

class UThreadNotifierPool final {
public:
    UThreadNotifierPool(UThreadEpollScheduler *const scheduler,
                        const int timeout_ms);
    ~UThreadNotifierPool();

    int GetNotifier(const std::string  &id, UThreadNotifier *&notifier);
    void ReleaseNotifier(const std::string &id);

private:
    std::map<std::string, UThreadNotifier *> pool_map_;
    UThreadEpollScheduler *scheduler_{nullptr};
    int timeout_ms_{5000};
};

class UThreadEpollScheduler final {
public:
    UThreadEpollScheduler(size_t stack_size, int max_task, const bool need_stack_protect = true);
    ~UThreadEpollScheduler();

    static UThreadEpollScheduler *Instance();

    bool IsTaskFull();

    //将任务添加到todo_list_中
    void AddTask(UThreadFunc_t func, void *args);

    UThreadSocket_t *CreateSocket(const int fd, const int socket_timeout_ms = 5000,
        const int connect_timeout_ms = 200, const bool no_delay = true);

    void SetActiveSocketFunc(UThreadActiveSocket_t active_socket_func);

    void SetHandlerAcceptedFdFunc(UThreadHandlerAcceptedFdFunc_t handler_accepted_fd_func);

    void SetHandlerNewRequestFunc(UThreadHandlerNewRequest_t handler_new_request_func);

    //切出任务
    bool YieldTask();

    /* Run函数先会将任务队列中的函数创建为协程，并Resume切换到协程，协程中会将fd的相应操作在epoll中注册，
     * 然后Yield回到回到Run. Run函数检查活动的fd，并Resume到活动的协程中进行IO操作. */
    bool Run();

    /* 首先调用EpollNotifier的Run函数，将Func函数加入调度器的任务队列，Func函数会读取管道，唤醒epoll.
    * 下一步会将任务队列中的函数创建为协程，并Resume切换到协程，协程中会将fd的相应操作在epoll中注册，
    * 然后Yield回到回到Run. Run函数检查活动的fd，并Resume到活动的协程中进行IO操作. */
    void RunForever();

    void Close();

    void NotifyEpoll();

    int GetCurrUThread();

    void AddTimer(UThreadSocket_t *socket, const int timeout_ms);
    void RemoveTimer(const size_t timer_id);
    void DealwithTimeout(int &next_timeout);

private:
    //任务队列
    typedef std::queue<std::pair<UThreadFunc_t, void *>> TaskQueue;

    /* 将任务队列中的函数创建为协程，并Resume切换到该协程，
     * 协程中会将fd相应的操作在epoll中注册然后Yield回到Run */
    void ConsumeTodoList();
    void ResumeAll(int flag);
    void StatEpollwaitEvents(const int event_count);

    UThreadRuntime runtime_;
    int max_task_;
    TaskQueue todo_list_;
    int epoll_fd_;

    Timer timer_;
    bool closed_{false};
    bool run_forever_{false};

    UThreadActiveSocket_t  active_socket_func_;
    UThreadHandlerAcceptedFdFunc_t handler_accepted_fd_func_;
    UThreadHandlerNewRequest_t handler_new_request_func_;

    int epoll_wait_events_;
    int epoll_wait_events_per_second_;
    uint64_t epoll_wait_events_last_cal_time_;

    EpollNotifier epoll_wake_up_;
};

class __uthread {
public:
    __uthread(UThreadEpollScheduler &scheduler) : scheduler_(scheduler) { }
    //重载-操作符，实现使用uthread_t将协程加入调度器的队列
    template <typename Func>
    void operator-(Func const &func) {
        scheduler_.AddTask(func, nullptr);
    }

private:
    UThreadEpollScheduler &scheduler_;
};

//以下定义了四个宏
//协程的准备
#define uthread_begin myrpc::UThreadEpollScheduler _uthread_scheduler(64 * 1024, 300);
#define uthread_begin_withargs(stack_size, max_task) myrpc::UThreadEpollScheduler _uthread_scheduler(stack_size, max_task);

//协程调度器的创建 
#define uthread_s _uthread_scheduler

//协程的创建
#define uthread_t myrpc::__uthread(_uthread_scheduler)-

//协程的结束
#define uthread_end _uthread_scheduler.Run();


//UThreadPoll负责注册对应事件之后Yield, Resume回来后删除注册
//接受一个socket的版本 
//epoll_ctl注册对应事件后Yield，Resume回来后删除注册
int UThreadPoll(UThreadSocket_t &socket, int events, int *revents, const int timeout_ms);

//接受一组socket的版本 
int UThreadPoll(UThreadSocket_t *list[], int count, const int timeout_ms);

int UThreadConnect(UThreadSocket_t &socket, const struct sockaddr *addr, socklen_t addrlen);

int UThreadAccept(UThreadSocket_t &socket, struct sockaddr *addr, socklen_t *addrlen);

ssize_t UThreadRecv(UThreadSocket_t &socket, void *buf, size_t len, const int flags);

ssize_t UThreadRead(UThreadSocket_t &socket, void *buf, size_t len, const int flags);

ssize_t UThreadSend(UThreadSocket_t &socket, const void *buf, size_t len, const int flags);

int UThreadClose(UThreadSocket_t &socket);

void UThreadSetConnectTimeout(UThreadSocket_t &socket, const int connect_timeout_ms);

void UThreadSetSocketTimeout(UThreadSocket_t &socket, const int socket_timeout_ms);

int UThreadSocketFd(UThreadSocket_t &socket);

size_t UThreadSocketTimerID(UThreadSocket_t &socket);

void UThreadSocketSetTimerID(UThreadSocket_t &socket, size_t timer_id);

UThreadSocket_t *NewUThreadSocket();

void UThreadSetArgs(UThreadSocket_t &socket, void *args);

void *UThreadGetArgs(UThreadSocket_t &socket);

void UThreadWait(UThreadSocket_t &socket, const int timeout_ms);

void UThreadLazyDestory(UThreadSocket_t &socket);

bool IsUThreadDestory(UThreadSocket_t &socket);

}
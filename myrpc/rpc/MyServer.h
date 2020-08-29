/* 包含整个RPC的基本结构.
 * DataFlow为数据流类，所有的请求和应答分别保存在两个线程安全的队列中.
 * Worker为工作线程类，如果是协程模式，每个Worker会有多个协程.
 * WorkerPool为工作线程池类，管理worker和调度各个工作线程.
 * MyServerUnit，独立的工作单元，每个单元有一个工作线程池，协程调度器和数据流.
 * MyServerIO，在MyServerUnit线程处理IO事件.
 * MyServer内含多个MyServerUnit工作单元.
 * MyServerAcceptor接受链接，工作在主线程中.
 * */

#pragma once 

#include "../http.h"
#include "../msg.h"
#include "ServerBase.h"
#include "ServerConfig.h"
#include "ThreadQueue.h"
#include <thread>

namespace myrpc {

class DataFlow final {
public:
    DataFlow();
    ~DataFlow();

    void PushRequest(void *args, BaseRequest *req);
    int PluckRequest(void *&args, BaseRequest *&req);
    int PickRequest(void *&args, BaseRequest *&req);

    void PushResponse(void *args, BaseResponse *resp);
    int PluckResponse(void *&args, BaseResponse *&resp);
    int PickResponse(void *&args, BaseResponse *&resp);

    bool CanPushRequest(const int max_queue_length);
    bool CanPushResponse(const int max_queue_length);

    bool CanPluckRequest();
    bool CanPluckResponse();

    void BreakOut();

private:
    struct QueueExtData {
        QueueExtData() {
            enqueue_time_ms = 0;
            args = nullptr;
        }

        QueueExtData(void *t_args) {
            enqueue_time_ms = Timer::GetSteadyClockMS();
            args = t_args;
        }

        uint64_t enqueue_time_ms;
        void *args;
    };

    ThreadQueue<std::pair<QueueExtData, BaseRequest *>> in_queue_;
    ThreadQueue<std::pair<QueueExtData, BaseResponse *>> out_queue_;
};

#define RPC_TIME_COST_CAL_RATE 1000
#define QUEUE_WAIT_TIME_COST_CAL_RATE 1000
#define MAX_QUEUE_WAIT_TIME_COST 500
#define MAX_ACCEPT_QUEUE_LENGTH 102400

class WorkerPool;

class Worker final {
public:
    Worker(const int idx, WorkerPool *const pool, 
        const int uthread_count, const int uthread_stack_size);
    ~Worker();

    void Func();
    void Shutdown();

    //线程模式
    void ThreadMode();

    //协程模式
    void UThreadMode();

    void HandlerNewRequestFunc();

    void UThreadFunc(void *args, BaseRequest *req, int queue_wait_time_ms);
    void WorkerLogic(void *args, BaseRequest *req, int queue_wait_time_ms);

    void NotifyEpoll();

private:
    int idx_ = -1;
    WorkerPool *pool_ = nullptr;
    int uthread_count_;
    int uthread_stack_size_;
    bool shut_down_ = false;
    UThreadEpollScheduler *worker_scheduler_ = nullptr;
    std::thread thread_;
};

typedef std::function <void (const BaseRequest &, BaseResponse *const, DispatcherArgs_t * const)> Dispatch_t;

class WorkerPool final {
public:
    WorkerPool (const int idx, UThreadEpollScheduler *const scheduler, const MyServerConfig *const config, 
        const int uthread_count, const int uthread_count_per_thread, const int uthread_stack_size, 
        DataFlow *const data_flow, Dispatch_t dispatch, void *args);
    ~WorkerPool();

    void NotifyEpoll();

private:
    friend class Worker;
    int idx_ = -1;
    UThreadEpollScheduler *scheduler_ = nullptr;
    const MyServerConfig *config_ = nullptr;
    DataFlow *data_flow_ = nullptr;
    Dispatch_t dispatch_;
    void *args_ = nullptr;
    std::vector<Worker *> worker_list_;
    size_t last_notify_idx_;
    std::mutex mutex_;
};

class MyServerIO final {
public:
    MyServerIO (const int idx, UThreadEpollScheduler *const scheduler, const MyServerConfig *config,
        DataFlow *data_flow, WorkerPool *worker_pool, myrpc::BaseMessageHandlerFactoryCreateFunc msg_handler_factory_create_func);
    ~MyServerIO();

    void RunForever();
    bool AddAcceptedFd(const int accepted_fd);
    void HandlerAcceptedFd();
    void IOFunc(int accept_fd);
    UThreadSocket_t *ActiveSocketFunc();

private:
    int idx_ = -1;
    UThreadEpollScheduler *scheduler_ = nullptr;
    const MyServerConfig *config_ = nullptr;
    DataFlow *data_flow_ = nullptr;
    WorkerPool *worker_pool_ = nullptr;
    std::unique_ptr<BaseMessageHandlerFactory> msg_handler_factory_;
    std::queue<int> accepted_fd_list_;
    std::mutex queue_mutex_;
};

class MyServer;

class MyServerUnit {
public:
    MyServerUnit(const int idx, MyServer *const my_server, int worker_thread_thread_count,
        int worker_uthread_count_per_thread, int worker_uthread_stack_size, Dispatch_t dispatch, void *args);
    virtual ~MyServerUnit();

    void RunFunc();
    bool AddAcceptedFd(const int accepted_fd);

private:
    MyServer *my_server_ = nullptr;
    UThreadEpollScheduler scheduler_;
    DataFlow data_flow_;
    WorkerPool worker_pool_;
    MyServerIO my_server_io_;
    std::thread thread_;
};

class MyServerAcceptor final {
public:
    MyServerAcceptor(MyServer *my_server);
    ~MyServerAcceptor();

    void LoopAccept(const char *const bind_ip, const int port);

private:
    MyServer *my_server_ = nullptr;
    size_t idx_ = 0;
};

class MyServer {
public:
    MyServer(const MyServerConfig &config, const Dispatch_t &dispatch, void *args, 
        myrpc::BaseMessageHandlerFactoryCreateFunc msg_handler_factory_create_func = 
        []()->std::unique_ptr<myrpc::HttpMessageHandlerFactory> { return std::unique_ptr<myrpc::HttpMessageHandlerFactory> 
        (new myrpc::HttpMessageHandlerFactory); });
    virtual ~MyServer();

    void RunForever();

private:
    friend class MyServerAcceptor;
    friend class MyServerUnit;

    const MyServerConfig *config_ = nullptr;
    myrpc::BaseMessageHandlerFactoryCreateFunc msg_handler_factory_create_func_;
    MyServerAcceptor my_server_acceptor_;
    std::vector<MyServerUnit *> server_unit_list_;
    
    void LoopReadCrossUnitResponse();
};

}
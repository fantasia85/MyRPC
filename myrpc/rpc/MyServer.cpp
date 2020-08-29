/* 包含整个RPC的基本结构.
 * DataFlow为数据流类，所有的请求和应答分别保存在两个线程安全的队列中.
 * Worker为工作线程类，如果是协程模式，每个Worker会有多个协程.
 * WorkerPool为工作线程池类，管理worker.
 * MyServerUnit，独立的工作单元，每个单元有一个工作线程池，协程调度器和数据流.
 * MyServerIO，在MyServerUnit线程处理IO事件.
 * MyServer内含多个MyServerUnit工作单元.
 * MyServerAcceptor接受链接，工作在主线程中.
 * */

#include "MyServer.h"
#include <assert.h>

namespace myrpc {

DataFlow::DataFlow() {

}

DataFlow::~DataFlow() {

}

void DataFlow::PushRequest(void *args, BaseRequest *req) {
    in_queue_.push(std::make_pair(QueueExtData(args), req));
}

int DataFlow::PluckRequest(void  *&args, BaseRequest *&req) {
    std::pair<QueueExtData, BaseRequest *> rp;
    bool succ = in_queue_.pluck(rp);
    if (!succ) 
        return 0;
    
    args = rp.first.args;
    req = rp.second;

    auto now_time = Timer::GetSteadyClockMS();
    return now_time > rp.first.enqueue_time_ms ? now_time - rp.first.enqueue_time_ms : 0;
}

int DataFlow::PickRequest(void *&args, BaseRequest *&req) {
    std::pair<QueueExtData, BaseRequest *> rp;
    bool succ = in_queue_.pick(rp);
    if (!succ) 
        return 0;

    args = rp.first.args;
    req = rp.second;

    auto now_time(Timer::GetSteadyClockMS());
    return now_time > rp.first.enqueue_time_ms ? now_time - rp.first.enqueue_time_ms : 0;
}

void DataFlow::PushResponse(void *args, BaseResponse *resp) {
    out_queue_.push(std::make_pair(QueueExtData(args), resp));
}

int DataFlow::PluckResponse(void *&args, BaseResponse *&resp) {
    std::pair<QueueExtData, BaseResponse *> rp;
    bool succ = out_queue_.pluck(rp);

    if (!succ) 
        return 0;

    args = rp.first.args;
    resp = rp.second;

    auto now_time = Timer::GetSteadyClockMS();

    return now_time > rp.first.enqueue_time_ms ? now_time - rp.first.enqueue_time_ms : 0;
}

int DataFlow::PickResponse(void *&args, BaseResponse *&resp) {
    std::pair<QueueExtData, BaseResponse *> rp;
    bool succ = out_queue_.pick(rp);

    if (!succ)
        return 0;

    args = rp.first.args;
    resp = rp.second;

    auto now_time(Timer::GetSteadyClockMS());

    return now_time > rp.first.enqueue_time_ms ? now_time - rp.first.enqueue_time_ms : 0;
}

bool DataFlow::CanPushRequest(const int max_queue_length) {
    return in_queue_.size() < (size_t) max_queue_length;
}

bool DataFlow::CanPushResponse(const int max_queue_length) {
    return out_queue_.size() < (size_t) max_queue_length;
}

bool DataFlow::CanPluckRequest() {
    return !in_queue_.empty();
}

bool DataFlow::CanPluckResponse() {
    return !out_queue_.empty();
}

void DataFlow::BreakOut() {
    in_queue_.break_out();
    out_queue_.break_out();
}


Worker::Worker(const int idx, WorkerPool *const pool, const int uthread_count, int uthread_stack_size)
    : idx_(idx), pool_(pool), uthread_count_(uthread_count),
      uthread_stack_size_(uthread_stack_size), thread_(&Worker::Func, this) {

}

Worker::~Worker() {
    thread_.join();
    delete worker_scheduler_;
}

void Worker::Func() {
    //如果thread_count为0，则为线程模式
    if (uthread_count_ == 0) 
        ThreadMode();
    else
        //否则为协程模式
        UThreadMode();
}

//在线程模式下，从队列中拉取请求，并在WorkerLogic函数中进行处理
void Worker::ThreadMode() {
    while (!shut_down_) {
        void *args = nullptr;
        BaseRequest *request = nullptr;
        
        int queue_wait_time_ms = pool_->data_flow_->PluckRequest(args, request);

        if (request == nullptr) 
            continue;

        WorkerLogic(args, request, queue_wait_time_ms);
    }
}

//在协程模式下，则会创建一个调度器，并设置处理新请求的函数
void Worker::UThreadMode() {
    worker_scheduler_ = new UThreadEpollScheduler(uthread_stack_size_, uthread_count_, true);
    assert(worker_scheduler_ != nullptr);
    worker_scheduler_->SetHandlerNewRequestFunc(std::bind(&Worker::HandlerNewRequestFunc, this));
    worker_scheduler_->RunForever();
}

//将WorkerLogic的包装加入调度器的任务队列
void Worker::HandlerNewRequestFunc() {
    if (worker_scheduler_->IsTaskFull())
        return;

    void *args = nullptr;
    BaseRequest *request = nullptr;
    int queue_wait_time_ms = pool_->data_flow_->PickRequest(args, request);

    if (!request)
        return;
    
    worker_scheduler_->AddTask(std::bind(&Worker::UThreadFunc, this, args, request, queue_wait_time_ms), nullptr);
}

void Worker::UThreadFunc(void *args, BaseRequest *req, int queue_wait_time_ms) {
    WorkerLogic(args, req, queue_wait_time_ms);
}

//真正处理逻辑的函数，对没有超时的请求，会分发到具体的函数处理，最后将结果push到response队列中
void Worker::WorkerLogic(void *args, BaseRequest *req, int queue_wait_time_ms) {
    BaseResponse *resp = req->GenResponse();

    if (queue_wait_time_ms < MAX_QUEUE_WAIT_TIME_COST) {
        DispatcherArgs_t dispatcher_args(worker_scheduler_, pool_->args_, args);
        pool_->dispatch_(*req, resp, &dispatcher_args);
    }

    pool_->data_flow_->PushResponse(args, resp);

    pool_->scheduler_->NotifyEpoll();

    if (req) {
        delete req;
        req = nullptr;
    }
}

void Worker::NotifyEpoll() {
    if (uthread_count_ == 0)
        return;

    worker_scheduler_->NotifyEpoll();
}

void Worker::Shutdown() {
    shut_down_ = true;
    pool_->data_flow_->BreakOut();
}

WorkerPool::WorkerPool(const int idx, UThreadEpollScheduler *const scheduler, const MyServerConfig *const config,
    const int thread_count, const int uthread_count_per_thread, const int uthread_stack_size, 
    DataFlow *const data_flow, Dispatch_t dispatch, void *args) 
    : idx_(idx), scheduler_(scheduler), config_(config), data_flow_(data_flow), dispatch_(dispatch),
      args_(args), last_notify_idx_(0) {
    for (int i = 0; i < thread_count; ++i) {
        auto worker = new Worker(i, this, uthread_count_per_thread, uthread_stack_size);
        assert(worker != nullptr);
        worker_list_.push_back(worker);
    }
}

WorkerPool::~WorkerPool() {
    for (auto &worker : worker_list_) {
        worker->Shutdown();
        delete worker;
    }
}

//采用轮询的方法来唤醒线程池中的一个工作线程
void WorkerPool::NotifyEpoll() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (last_notify_idx_ == worker_list_.size())
        last_notify_idx_ = 0;

    worker_list_[last_notify_idx_++]->NotifyEpoll();
}

MyServerIO::MyServerIO(const int idx, UThreadEpollScheduler *const scheduler, const MyServerConfig *config,
    DataFlow *data_flow, WorkerPool *worker_pool, myrpc::BaseMessageHandlerFactoryCreateFunc msg_handler_factory_create_func)
    : idx_(idx), scheduler_(scheduler), config_(config), data_flow_(data_flow), worker_pool_(worker_pool),
      msg_handler_factory_(std::move(msg_handler_factory_create_func())) {

}

MyServerIO::~MyServerIO() {

}

bool MyServerIO::AddAcceptedFd(const int accepted_fd) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (accepted_fd_list_.size() > MAX_ACCEPT_QUEUE_LENGTH) 
        return false;
    accepted_fd_list_.push(accepted_fd);

    scheduler_->NotifyEpoll();

    return true;
}

void MyServerIO::HandlerAcceptedFd() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!accepted_fd_list_.empty()) {
        int accepted_fd = accepted_fd_list_.front();
        accepted_fd_list_.pop();
        scheduler_->AddTask(std::bind(&MyServerIO::IOFunc, this, accepted_fd), nullptr);
    }
}

void MyServerIO::IOFunc(int accepted_fd) {
    UThreadSocket_t *socket = scheduler_->CreateSocket(accepted_fd);
    UThreadTcpStream stream;
    stream.Attach(socket);
    UThreadSetSocketTimeout(*socket, config_->GetSocketTimeoutMS());

    while (true) {
        auto msg_handler(msg_handler_factory_->Create());
        if (!msg_handler) {
            //log
            break;
        }

        //会在worker中被删除
        BaseRequest *req = nullptr;
        int ret = msg_handler->RecvRequest(stream, req);
        if (ret != 0) {
            if (req) {
                delete req;
                req = nullptr;
            }

            //log

            break;
        }

        char client_ip[128] = {'\0'};
        stream.GetRemoteHost(client_ip, sizeof(client_ip));
        //log

        if (!data_flow_->CanPushRequest(config_->GetMaxQueueLength())) {
            if (req) {
                delete req;
                req = nullptr;
            }

            //log

            break;
        }

        data_flow_->PushRequest(socket, req);

        //如果工作线程工作在协程模式，则需要唤醒
        //工作线程会删除掉request
        worker_pool_->NotifyEpoll();
        UThreadSetArgs(*socket, nullptr);

        UThreadWait(*socket, config_->GetSocketTimeoutMS());
        if (UThreadGetArgs(*socket) == nullptr) {
            //因为有一个输入队列，所以弹出后套接字会被关闭
            socket = stream.DetachSocket();
            UThreadLazyDestory(*socket);

            //log

            break;
        }

        {
            BaseResponse *resp = (BaseResponse *) UThreadGetArgs(*socket);
            if (!resp->fake()) {
                ret = resp->Send(stream);
                //log
            }
            delete resp;
        }

        if (!msg_handler->keep_alive() || (ret != 0)) 
            break;
    }
}

UThreadSocket_t *MyServerIO::ActiveSocketFunc() {
    while (data_flow_->CanPluckResponse()) {
        void *args = nullptr;
        BaseResponse *resp = nullptr;

        int queue_wait_time_ms = data_flow_->PluckResponse(args, resp);
        if (!resp)
            return nullptr;

        UThreadSocket_t *socket = (UThreadSocket_t *) args;
        //套接字已经超时，关闭套接字
        if (socket != nullptr && IsUThreadDestory(*socket)) {
            UThreadClose(*socket);
            free(socket);
            delete resp;

            continue;
        }

        UThreadSetArgs(*socket, (void *)resp);

        return socket;
    }

    return  nullptr;
}

void MyServerIO::RunForever() {
    scheduler_->SetHandlerAcceptedFdFunc(std::bind(&MyServerIO::HandlerAcceptedFd, this));
    scheduler_->SetActiveSocketFunc(std::bind(&MyServerIO::ActiveSocketFunc, this));
    scheduler_->RunForever();
}

/* 
 * points to incompliate class 
MyServerUnit::MyServerUnit(const int idx, MyServer *const my_server, int worker_thread_count,
    int worker_uthread_count_per_thread, int worker_uthread_stack_size, Dispatch_t dispatch, void *args)
    : my_server_(my_server), scheduler_(8 * 1024, 1000000, false), 
      worker_pool_(idx, &scheduler_, my_server_->config_, worker_thread_count, worker_uthread_count_per_thread,
        worker_uthread_stack_size, &data_flow_, dispatch, args),
      my_server_io_(idx, &scheduler_, my_server_->config_, &data_flow_, &worker_pool_,
        my_server_->msg_handler_factory_create_func_),
      thread_(&MyServerUnit::RunFunc, this) {

}
*/

MyServerUnit::~MyServerUnit() {
    thread_.join();
}

void MyServerUnit::RunFunc() {
    my_server_io_.RunForever();
}

bool MyServerUnit::AddAcceptedFd(const int accepted_fd) {
    return my_server_io_.AddAcceptedFd(accepted_fd);
}

}
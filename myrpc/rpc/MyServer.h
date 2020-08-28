/* 包含整个RPC的基本结构.
 * DataFlow为数据流类，所有的请求和应答分别保存在两个线程安全的队列中.
 * Worker为工作线程类，如果是协程模式，每个Worker会有多个协程.
 * WorkerPool为工作线程池类，管理worker.
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

class Worker final {

private:

};

}
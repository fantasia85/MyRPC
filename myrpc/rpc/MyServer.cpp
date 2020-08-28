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

}
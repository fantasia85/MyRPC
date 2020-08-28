/* 定义了DispatcherArgs_t */

#pragma once

#include "../network.h"

namespace myrpc {

class DataFlow;

typedef struct tagDispatcherArgs {
    //服务器的工作协程调度器
    UThreadEpollScheduler *server_worker_uthread_scheduler = nullptr;
    void *server_args = nullptr;
    void *data_flow_args = nullptr;

    tagDispatcherArgs(UThreadEpollScheduler *const server_worker_uthread_scheduler_value,
                    void *const service_args_value, void *const data_flow_args_value) 
                    : server_worker_uthread_scheduler(server_worker_uthread_scheduler_value),
                      server_args(service_args_value), data_flow_args(data_flow_args_value) {

    }
    
} DispatcherArgs_t;

}
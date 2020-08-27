/* 用于配置服务器的各项配置 */

#include "ServerConfig.h"
#include <cstring>
#include <stdio.h>

namespace myrpc {

ServerConfig::ServerConfig() {
    bzero(section_name_prefix_, sizeof(section_name_prefix_));
    bzero(bind_ip_, sizeof(bind_ip_));
    port_ = -1;
    max_threads_ = 120;
    socket_timeout_ms_ = 30000;
    bzero(package_name_, sizeof(package_name_));
}

ServerConfig::~ServerConfig() {

}

void ServerConfig::set_section_name_prefix(const char *section_name_prefix) {
    strncpy(section_name_prefix_, section_name_prefix, sizeof(section_name_prefix_) - 1);
}

const char *ServerConfig::section_name_prefix() const {
    return section_name_prefix_;
}

void ServerConfig::SetBindIP(const char *ip) {
    snprintf(bind_ip_, sizeof(bind_ip_), "%s", ip);
}

const char *ServerConfig::GetBindIP() const {
    return bind_ip_;
}

void ServerConfig::SetPort(int port) {
    port_ = port;
}

int ServerConfig::GetPort() const {
    return port_;
}

void ServerConfig::SetMaxThreads(int max_threads) {
    max_threads_ = max_threads;
}

int ServerConfig::GetMaxThreads() const {
    return max_threads_;
}

void ServerConfig::SetSocketTimeoutMS(int socket_timeout_ms) {
    socket_timeout_ms_ = socket_timeout_ms;
}

int ServerConfig::GetSocketTimeoutMS() const {
    return socket_timeout_ms_;
}

void ServerConfig::SetPackageName(const char *package_name) {
    strncpy(package_name_, package_name, sizeof(package_name_) - 1);
}

const char *ServerConfig::GetPackageName() const {
    return package_name_;
}

MyServerConfig::MyServerConfig()
    : max_connections_(800000), max_queue_length_(20480), io_thread_count_(3),
      worker_uthread_count_(0), worker_uthread_stack_size_(64 * 1024) {

}

MyServerConfig::~MyServerConfig() {

}

void MyServerConfig::SetMaxConnections(const int max_connections) {
    max_connections_ = max_connections;
}

int MyServerConfig::GetMaxConnections() const {
    return max_connections_;
}

void MyServerConfig::SetMaxQueueLength(const int max_queue_length) {
    max_queue_length_ = max_queue_length;
}

int MyServerConfig::GetMaxQueueLength() const {
    return max_queue_length_;
}

void MyServerConfig::SetIOThreadCount(const int io_thread_count) {
    io_thread_count_ = io_thread_count;
}

int MyServerConfig::GetIOThreadCount() const {
    return io_thread_count_;
}

void MyServerConfig::SetWorkerUThreadCount(const int worker_uthread_count) {
    worker_uthread_count_ = worker_uthread_count;
}

int MyServerConfig::GetWorkerUThreadCount() const {
    return  worker_uthread_count_;
}

void MyServerConfig::SetWorkerUThreadStackSize(const int worker_uthread_stack_size) {
    worker_uthread_stack_size_ = worker_uthread_stack_size_;
}

int MyServerConfig::GetWorkerUThreadStackSize() const {
    return worker_uthread_stack_size_;
}

}
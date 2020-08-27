/* 用于配置服务器的各项配置 */

#pragma once 

namespace myrpc {

class ServerConfig {
public:
    ServerConfig();
    virtual ~ServerConfig();

    //Read
    //DoRead

    void SetBindIP(const char *ip);
    const char *GetBindIP() const;

    void SetPort(int port);
    int GetPort() const;

    void SetMaxThreads(int max_threads);
    int GetMaxThreads() const;

    void SetSocketTimeoutMS(int socket_timeout_ms);
    int GetSocketTimeoutMS() const;

    void SetPackageName(const char *package_name);
    const char *GetPackageName() const;

    //GetLogDir
    //SetLogLevel
    //GetLogLevel

protected:
    void set_section_name_prefix(const char *section_name_predix);
    const char *section_name_prefix() const;

private:
    char section_name_prefix_[64];
    char bind_ip_[32];
    int port_;
    int max_threads_;
    int socket_timeout_ms_;
    char package_name_[64];
    //char log_dir_[128];
    //int log_level_;
};

class MyServerConfig : public ServerConfig {
public:
    MyServerConfig();
    virtual ~MyServerConfig() override;

    //DoRead

    void SetMaxConnections(const int max_connections);
    int GetMaxConnections() const;

    void SetMaxQueueLength(const int max_queue_length);
    int GetMaxQueueLength() const;

    //SetFastRejectThresholdMS
    //GetFastRejectThresholdMS

    //SetFastRejectAdjustRate
    //GetFastRejectAdjustRate

    void SetIOThreadCount(const int io_thread_count);
    int GetIOThreadCount() const;

    void SetWorkerUThreadCount(const int worker_uthread_count);
    int GetWorkerUThreadCount() const;

    void SetWorkerUThreadStackSize(const int worker_uthread_stack_size);
    int GetWorkerUThreadStackSize() const;

private:
    int max_connections_;
    int max_queue_length_;
    //int fast_reject_threshold_ms_;
    //int fast_reject_adjust_rate_;
    int io_thread_count_;
    int worker_uthread_count_;
    int worker_uthread_stack_size_;
};

}
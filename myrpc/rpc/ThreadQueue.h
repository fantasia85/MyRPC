/* 线程间请求队列和响应队列的基本结构，以标准库的queue作为
 * 底层的数据结构，用mutex和condition_variable实现线程间的
 * 同步机制 */

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace myrpc {

template <class T>
class ThreadQueue {
public:
    ThreadQueue() : break_out_(false), size_(0) { }
    ~ThreadQueue() {
        break_out();
    }

    size_t size() {
        return static_cast<size_t> (size_);
    }

    bool empty() {
        return static_casr<size_t>(size_) == 0;
    }

    //压入数据，并提醒有新的数据被压入
    void push(const T &value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        size_++;
        cv_.notify_one();
    }

    //提取数据，当队列为空时，会等待直到由数据为止
    bool pluck(T &value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (break_out_) 
            return false;
        while (queu_.empty()) {
            cv_.wait(lock);
            if (break_out_)
                return false;
        }

        size_--;
        value = queue_.front();
        queue_.pop();
        return  true;
    }

    bool pick(T &value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return  false;
        }

        size_--;
        value = queue_.front();
        queue_.pop();
        return true;    
    }

    void break_out() {
        std::lock_guard<std::mutex> lock(mutex_);
        break_out_ = true;
        //析构时唤醒所有等待的线程
        cv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool break_out_;
    std::atomic_int size_;
};

}
#ifndef THREAD_COND_HPP
#define THREAD_COND_HPP

#include "common.h"
#include "thread_mutex.hpp"

namespace multi_thread {

class ThreadCondition {
public:
    explicit ThreadCondition(ThreadMutex* thread_mutex);
    ~ThreadCondition();
    int thread_wait();
    int thread_signal();
    int thread_broadcast();
    //pthread_cond_t* get_thread_cond() const;
    bool get_cond_status() const;

protected:
    void initialization(ThreadMutex* thread_mutex);
    void destroy();

private:
    ThreadMutex* _thread_mutex;
    pthread_cond_t _thread_cond;
    bool _is_create;
};

ThreadCondition::ThreadCondition(ThreadMutex* thread_mutex) {
    _is_create = false;
    initialization(thread_mutex);
}

ThreadCondition::~ThreadCondition() {
    int count_down = 5;
    while (_is_create == true && count_down > 0) {
        destroy();
        count_down -= 1;
    }
    _thread_mutex = NULL;
}

void ThreadCondition::initialization(ThreadMutex* thread_mutex) {
    if (thread_mutex != NULL) {
        _thread_mutex = thread_mutex;
        int return_val = pthread_cond_init(&_thread_cond, NULL);
        if (return_val != 0) {
            cout << "Condition initialization failed." << endl;
            _is_create = false;
        } else {
            _is_create = true;
        }
    } else {
        _is_create = false;
    }
}

void ThreadCondition::destroy() {
    int return_val = pthread_cond_destroy(&_thread_cond);
    if (return_val != 0) {
        cout << "Condition destroy failed." << endl;
        _is_create = true;
    } else {
        _is_create = false;
    }
}

int ThreadCondition::thread_wait() {
    if (_is_create == true) {
        return pthread_cond_wait(&_thread_cond, _thread_mutex->get_thread_mutex());
    } else {
        return -1;
    }
}

int ThreadCondition::thread_signal() {
    if (_is_create == true) {
        return pthread_cond_signal(&_thread_cond);
    } else {
        return -1;
    }
}

int ThreadCondition::thread_broadcast() {
    if (_is_create == true) {
        return pthread_cond_broadcast(&_thread_cond);
    } else {
        return -1;
    }
}

bool ThreadCondition::get_cond_status() const {
    return _is_create;
}

} // namespace multi_thread

#endif // thread_cond.hpp
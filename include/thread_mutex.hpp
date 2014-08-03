#ifndef THREAD_MUTEX_HPP
#define THREAD_MUTEX_HPP

#include "common.h"

namespace multi_thread {

void mutex_cleanup(void *arg) {
    pthread_mutex_unlock((pthread_mutex_t*)arg);
}

class ThreadMutex {
public:
    ThreadMutex();
    ~ThreadMutex();
    int lock();
    int unlock();
    pthread_mutex_t* get_thread_mutex();
    bool get_mutex_status () const;
    //void cleanup_pop();
    //void cleanup_push();

protected:
    void initialization();
    void destroy();
    //void cleanup(void *arg);

private:
    pthread_mutex_t _thread_mutex;
    bool _is_create;
};

ThreadMutex::ThreadMutex() {
    _is_create = false;
    initialization();
}

ThreadMutex::~ThreadMutex() {
    int count_down = 5;
    while (_is_create == true && count_down > 0) {
        destroy();
        count_down -= 1;
    }
}

void ThreadMutex::initialization() {
    int return_val = pthread_mutex_init(&_thread_mutex, NULL);
    if (return_val != 0) {
        cout << "Mutex initialization failed." << endl;
        _is_create = false;
    } else {
        _is_create = true;
    }
}

void ThreadMutex::destroy() {
    int return_val = pthread_mutex_destroy(&_thread_mutex);
    if (return_val != 0) {
        cout << "Mutex destroy failed(code: " << return_val << " )" << endl;
        _is_create = true;
    } else {
        _is_create = false;
    }
}

/*
void ThreadMutex::cleanup_push() {
    pthread_cleanup_push(pthread_mutex_unlock, (void*)&_thread_mutex);
}

void ThreadMutex::cleanup_pop() {
    pthread_cleanup_pop(0);
}
*/

int ThreadMutex::lock() {
    if (_is_create == true) {
        return pthread_mutex_lock(&_thread_mutex);
    } else {
        int count_down = 5;
        while (count_down > 0 && _is_create == false) {
            initialization();
            count_down -= 1;
        } 
        if (_is_create == true) {
            return pthread_mutex_lock(&_thread_mutex);
        } else {
            return -1;
        }
    }
}

int ThreadMutex::unlock() {
    if (_is_create == true) {
        return pthread_mutex_unlock(&_thread_mutex);
    } else {
        int count_down = 5;
        while (count_down > 0 && _is_create == false) {
            initialization();
            count_down -= 1;
        } 
        if (_is_create == true) {
            return pthread_mutex_unlock(&_thread_mutex);
        } else {
            return -1;
        }
    }
}

pthread_mutex_t* ThreadMutex::get_thread_mutex() {
    if (_is_create == true) {  
        return &_thread_mutex;
    } else {
        return NULL;
    }
}

bool ThreadMutex::get_mutex_status() const {
    return _is_create;
}

}



#endif // thread_mutex.hpp
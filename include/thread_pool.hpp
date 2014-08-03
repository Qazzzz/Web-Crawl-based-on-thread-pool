#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "common.h"
#include "thread_mutex.hpp"
#include "thread_cond.hpp"

// TODO: make the threadpool's thread capacity adjustment better
// TODO: reduce the time of synchronous

namespace multi_thread {

class ThreadPool;
class Job;

class Thread {
public:
    Thread();
    virtual ~Thread();
    virtual void run() = 0;
    virtual int run_thread();
    virtual void wait_thread_exit();
    pthread_t get_thread_id() const;
    int get_thread_status() const;
    void set_thread_state(int state);

    static const int _s_thread_status_new = 0;  
    static const int _s_thread_status_busy = 1;  
    static const int _s_thread_status_exit = -1;

private:
    pthread_t _thread_id;
    int _thread_status;
    static void* thread_agent_func(void *args);
};


//------------------------------------------
class WorkerThread: public Thread {
public:
    WorkerThread();
    virtual ~WorkerThread();

    Job* get_job() const;
    void set_job(Job* job, void* job_data);
    void set_thread_pool(ThreadPool* thread_pool);
    void run();

private:
    ThreadPool* _thread_pool;
    Job* _job;
    void* _job_data;

    ThreadMutex _pthread_mutex_worker_thread;
    ThreadMutex _pthread_mutex_data;
    ThreadCondition _pthread_cond_run;
};


//------------------------------------------
class Job {
public:
    Job();
    virtual ~Job();

    int get_job_no() const;
    void set_job_no(int job_no);
    string get_job_name() const;
    void set_job_name(string &job_name);
    WorkerThread* get_worker_thread() const;
    void set_worker_thread(WorkerThread* worker_thread);

    virtual void run(void* job_pointer) = 0;

private:
    int _job_no;
    string _job_name;
    WorkerThread *_worker_thread;
};


//------------------------------------------
class ThreadPool {
public:
    ThreadPool();
    explicit ThreadPool(int thread_num);
    virtual ~ThreadPool();

    int get_max_thread_idle() const;
    int get_min_thread_idle() const;
    int get_init_thread() const;
    int get_available_thread();
    int get_running_thread();
    int get_current_thread();
    void set_max_thread_idle(int max_thread_idle);

    void move_to_busy_list(WorkerThread* idle_thread);
    void move_to_idle_list(WorkerThread* busy_thread);
    void add_to_idle_list(WorkerThread* new_idle_thread);
    void delete_idle_thread(int thread_num);
    void destroy_all_thread();

    void run(Job* job, void* job_data);

protected:
    WorkerThread* get_idle_thread();
    void create_idle_thread(int thread_num);
    
private:
    int _init_thread;
    int _current_thread;
    int _busy_thread;
    int _max_thread_idle;
    int _min_thread_idle;
    int _available_thread;   // _idle_thread_list.size()
    int _exist_thread;
    
    vector<WorkerThread*> _idle_thread_list;
    vector<WorkerThread*> _busy_thread_list;
    
    ThreadMutex _pthread_mutex_idle;
    ThreadMutex _pthread_mutex_busy;
    ThreadMutex _pthread_mutex_max;
    ThreadMutex _pthread_mutex_exist_thread;
    ThreadCondition _pthread_cond_max;
};


/*-------------------------------
            Class Thread
 -------------------------------*/
Thread::Thread() {
    _thread_id = 0;
    _thread_status = _s_thread_status_new;
}

Thread::~Thread() {

}

void* Thread::thread_agent_func(void *args) {
    WorkerThread *thread_this = static_cast<WorkerThread*>(args);
    //pthread_setcancletype(PTHREAD_CANCEL_DEFERRED);
    thread_this->run();
    //delete thread_this;
    pthread_exit(NULL);
}

int Thread::run_thread() {
    int err = pthread_create(&_thread_id, NULL, thread_agent_func, this);
    _thread_status = _s_thread_status_busy;
    if (err != 0) {
        cout << "Thread create error." << endl;
        return -1;
    } else {
        return 1;
    }
}

void Thread::wait_thread_exit() {
    if (_thread_id != 0) {
        //cout << "delete thread." << endl;
        pthread_cancel(_thread_id);
        void* return_value;
        pthread_join(_thread_id, &return_value);
        _thread_status = _s_thread_status_exit;
        _thread_id = 0;
        //cout << "thread has been deleted." << endl;
    } else {
        cout << "Thread has not been initialized." << endl;
    }
}

pthread_t Thread::get_thread_id() const {
    return _thread_id;
}

int Thread::get_thread_status() const {
    return _thread_status;
}

void Thread::set_thread_state(int state) {
    _thread_status = state;
}


/*-------------------------------
        Class WorkerThread
 -------------------------------*/
WorkerThread::WorkerThread() : 
    _pthread_mutex_worker_thread(),
    _pthread_mutex_data(),
    _pthread_cond_run(&_pthread_mutex_worker_thread),
    _job(NULL),
    _job_data(NULL),
    _thread_pool(NULL) {
}

WorkerThread::~WorkerThread() {
    if (_job != NULL) {
        delete _job;
    }
    if (_thread_pool != NULL) {
        _thread_pool = NULL;
    }
}

// TODO: error handle in mutex operation
void WorkerThread::run() {
    set_thread_state(_s_thread_status_busy);
    pthread_cleanup_push(mutex_cleanup, 
                         static_cast<void*>(_pthread_mutex_worker_thread.get_thread_mutex()));
    while (1) {

        _pthread_mutex_worker_thread.lock();
        while (_job == NULL) {
            _pthread_cond_run.thread_wait();
        }

        //cout << *(int*)(_job_data) << endl;
        //cout << this << *(int*)(_job_data) << endl;
        _job->run(_job_data);

        _job->set_worker_thread(NULL);
        _job = NULL;
        _job_data = NULL;
        _thread_pool->move_to_idle_list(this);
        if (_thread_pool->get_available_thread() > _thread_pool->get_max_thread_idle()) {
            cout << "available thread > max idle thread" << endl;
            _thread_pool->delete_idle_thread(_thread_pool->get_available_thread() -
                                               _thread_pool->get_init_thread());
        }
        _pthread_mutex_worker_thread.unlock();
        
    }
    pthread_cleanup_pop(0);
}

Job* WorkerThread::get_job() const {
    return _job;
}

void WorkerThread::set_job(Job* job, void* job_data) {
    _pthread_mutex_data.lock();
    _job = job;
    _job_data = job_data;
    _job->set_worker_thread(this);
    _pthread_mutex_data.unlock();

    _pthread_mutex_worker_thread.lock();
    _pthread_cond_run.thread_signal();
    _pthread_mutex_worker_thread.unlock();
}

void WorkerThread::set_thread_pool(ThreadPool* thread_pool) {
    _pthread_mutex_data.lock();
    _thread_pool = thread_pool;
    _pthread_mutex_data.unlock();
}


/*-------------------------------
        Class Job
 -------------------------------*/
Job::Job() {
    _job_no = 0;
    _worker_thread = NULL;
}

Job::~Job() {
    _worker_thread = NULL;
}

int Job::get_job_no() const {
    return _job_no;
}

void Job::set_job_no(int job_no) {
    _job_no = job_no;
}

string Job::get_job_name() const {
    return _job_name;
}

void Job::set_job_name(string &job_name) {
    _job_name = job_name;
}

WorkerThread* Job::get_worker_thread() const {
    return _worker_thread;
}

void Job::set_worker_thread(WorkerThread* worker_thread) {
    _worker_thread = worker_thread;
}

/*-------------------------------
        Class ThreadPool
 -------------------------------*/
ThreadPool::ThreadPool() : 
    _pthread_mutex_idle(),
    _pthread_mutex_busy(),
    _pthread_mutex_max(),
    _pthread_mutex_exist_thread(),
    _pthread_cond_max(&_pthread_mutex_max) {
    _max_thread_idle = 40;
    _min_thread_idle = 5;
    _init_thread = _available_thread = 20;
    _busy_thread = 0;
    _exist_thread = 0;

    _idle_thread_list.clear();
    _busy_thread_list.clear();
    create_idle_thread(_init_thread);
}

ThreadPool::ThreadPool(int thread_num) :
    _pthread_mutex_idle(),
    _pthread_mutex_busy(),
    _pthread_mutex_max(),
    _pthread_mutex_exist_thread(),
    _pthread_cond_max(&_pthread_mutex_max) {
    _max_thread_idle = 40;
    _busy_thread = 0;
    if (thread_num > _max_thread_idle) {
        _init_thread = _max_thread_idle;
    } else if (thread_num < 5) {
        _init_thread = 5;
    } else {
        _init_thread = thread_num;
    }
    _available_thread = _init_thread;
    _min_thread_idle = _init_thread - 10 > 5 ? _init_thread - 10 : 5;
    _exist_thread = 0;

    create_idle_thread(_init_thread);
}

ThreadPool::~ThreadPool() {
    destroy_all_thread();
}

void ThreadPool::destroy_all_thread() {
    WorkerThread* tmp_idle_thread_ptr;
    vector<WorkerThread*>::iterator pos;
    while(1) {
        _pthread_mutex_idle.lock();
        while(!_idle_thread_list.empty()) {
            tmp_idle_thread_ptr = _idle_thread_list.front();
            _pthread_mutex_idle.unlock();

            tmp_idle_thread_ptr->wait_thread_exit();

            _pthread_mutex_idle.lock();
            pos = find(_idle_thread_list.begin(), _idle_thread_list.end(), tmp_idle_thread_ptr);
            if (pos != _idle_thread_list.end()) {
                delete *pos;
                *pos = NULL;
                _idle_thread_list.erase(pos);
            }
            _pthread_mutex_exist_thread.lock();
            _exist_thread -= 1;
            //cout << _exist_thread << endl;
            //cout << "idle: " << _idle_thread_list.size() << endl;
            //cout << "busy: " << _busy_thread_list.size() << endl;
            _pthread_mutex_exist_thread.unlock();
        }
        _pthread_mutex_idle.unlock();
        tmp_idle_thread_ptr = NULL;

        _pthread_mutex_exist_thread.lock();
        if (_exist_thread == 0) {
            _pthread_mutex_exist_thread.unlock();
            //cout << "empty" << endl;
            break;
        }
        _pthread_mutex_exist_thread.unlock();
    }
}

void ThreadPool::move_to_busy_list(WorkerThread* idle_thread) {
    _pthread_mutex_idle.lock();
    vector<WorkerThread*>::iterator pos;
    pos = find(_idle_thread_list.begin(), _idle_thread_list.end(), idle_thread);
    if (pos != _idle_thread_list.end()) {
        _idle_thread_list.erase(pos);
    }
    _available_thread = _idle_thread_list.size();
    _pthread_mutex_idle.unlock();

    _pthread_mutex_busy.lock();
    _busy_thread_list.push_back(idle_thread);
    _busy_thread = _busy_thread_list.size();
    _pthread_mutex_busy.unlock();
}

void ThreadPool::move_to_idle_list(WorkerThread* busy_thread) {
    _pthread_mutex_idle.lock();
    _idle_thread_list.push_back(busy_thread);
    _available_thread = _idle_thread_list.size();
    _pthread_mutex_idle.unlock();

    _pthread_mutex_busy.lock();
    vector<WorkerThread*>::iterator pos;
    pos = find(_busy_thread_list.begin(), _busy_thread_list.end(), busy_thread);
    if (pos != _busy_thread_list.end()) {
        _busy_thread_list.erase(pos);
    }
    _busy_thread = _busy_thread_list.size();
    _pthread_mutex_busy.unlock();

    _pthread_cond_max.thread_signal();
}

void ThreadPool::add_to_idle_list(WorkerThread* new_idle_thread) {
    _pthread_mutex_idle.lock();
    _idle_thread_list.push_back(new_idle_thread);
    _available_thread = _idle_thread_list.size();
    _pthread_mutex_idle.unlock();
}

void ThreadPool::delete_idle_thread(int thread_num) {
    _pthread_mutex_idle.lock();
    for (int i = 0; i < thread_num; i++) {
        if (_idle_thread_list.size() > 0) {
            _idle_thread_list.front() -> wait_thread_exit();
            delete _idle_thread_list.front();
            _idle_thread_list.front() = NULL;
            _idle_thread_list.erase(_idle_thread_list.begin());

        } else {
            break;
        }
    }
    _available_thread = _idle_thread_list.size();
    _pthread_mutex_idle.unlock();

    _pthread_mutex_exist_thread.lock();
    _exist_thread -= thread_num;
    //cout << "del thread " << thread_num << endl;
    _pthread_mutex_exist_thread.unlock();
}

void ThreadPool::create_idle_thread(int thread_num) {
    for (int i = 0; i < thread_num; i++) {
        WorkerThread* worker_thread = new WorkerThread;
        add_to_idle_list(worker_thread);
        worker_thread->set_thread_pool(this);
        worker_thread->run_thread();
    }
    _pthread_mutex_exist_thread.lock();
    _exist_thread += thread_num;
    //cout << "new thread " << thread_num << endl;
    _pthread_mutex_exist_thread.unlock();
}

void ThreadPool::run(Job* job, void* job_data) {
    if (job != NULL) {
        if (get_available_thread() == 0) {
            //cout << "no available thread" << endl;
            if (get_current_thread() < get_max_thread_idle()) {
                //cout << "create new idle thread" << endl;
                create_idle_thread((get_max_thread_idle() - get_current_thread())/2);
            } else {
                //cout << "wait for new thread" << endl;
                _pthread_mutex_max.lock();
                _pthread_cond_max.thread_wait();
                _pthread_mutex_max.unlock();
            }
        }

        WorkerThread *worker_thread = get_idle_thread();
        if (worker_thread != NULL) {
            //cout << worker_thread << endl;
            move_to_busy_list(worker_thread);
            worker_thread->set_thread_pool(this);
            job->set_worker_thread(worker_thread);
            //cout << job << job_data << endl;
            worker_thread->set_job(job, job_data);
        }
    } else {
        cout << "Illegal Job." << endl;
    }
}

WorkerThread* ThreadPool::get_idle_thread() {
    _pthread_mutex_idle.lock();
    if (_idle_thread_list.size() > 0) {
        WorkerThread* worker_thread = _idle_thread_list.front();
        _idle_thread_list.erase(_idle_thread_list.begin());
        _available_thread = _idle_thread_list.size();
        _pthread_mutex_idle.unlock();
        return worker_thread;
    } else {
        _pthread_mutex_idle.unlock();
        return NULL;
    }
}

int ThreadPool::get_current_thread() {
    _pthread_mutex_idle.lock();
    _pthread_mutex_busy.lock();
    _current_thread = _idle_thread_list.size() + _busy_thread_list.size();
    _pthread_mutex_busy.unlock();
    _pthread_mutex_idle.unlock();
    return _current_thread;
}

int ThreadPool::get_max_thread_idle() const {
    return _max_thread_idle;
}

int ThreadPool::get_min_thread_idle() const {
    return _min_thread_idle;
}

int ThreadPool::get_init_thread() const {
    return _init_thread;
}

// Equal to amounts of idle threads
int ThreadPool::get_available_thread() {
    _pthread_mutex_idle.lock();
    _available_thread = _idle_thread_list.size();
    _pthread_mutex_idle.unlock();
    return _available_thread;
}

// Equal to amounts of busy threads
int ThreadPool::get_running_thread() {
    _pthread_mutex_busy.lock();
    _busy_thread = _busy_thread_list.size();
    _pthread_mutex_busy.unlock();
    return _busy_thread;
}

void ThreadPool::set_max_thread_idle(int max_thread_idle) {
    if (max_thread_idle > 20) {
        _max_thread_idle = max_thread_idle;
    }
}

} // multi_thread

#endif // thread_pool.hpp
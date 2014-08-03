#include "../include/thread_cond.hpp"
#include "../include/thread_mutex.hpp"
#include "gtest/gtest.h"

multi_thread::ThreadMutex thread_mutex_wait_signal;
multi_thread::ThreadMutex thread_mutex_wait_signal_2;
multi_thread::ThreadCondition thread_cond_wait_signal(&thread_mutex_wait_signal);
multi_thread::ThreadCondition thread_cond_wait_signal_2(&thread_mutex_wait_signal_2);
int a;

void *cond_test_wait_signal(void *args) {
    thread_mutex_wait_signal.lock();
    a++;
    thread_cond_wait_signal_2.thread_signal();
    thread_cond_wait_signal.thread_wait();
    a++;
    thread_mutex_wait_signal.unlock();
    pthread_exit(NULL);
}

TEST(ThreadMCondTest, thread_cond_test_wait_signal) {
    thread_mutex_wait_signal_2.lock();
    pthread_t thread_id;
    a = 1;
    int return_value = pthread_create(&thread_id, NULL, cond_test_wait_signal, NULL);
    if (return_value != 0) {
        cout << "create thread error." << endl;
    }

    thread_cond_wait_signal_2.thread_wait();

    EXPECT_EQ(2, a);
    thread_cond_wait_signal.thread_signal();
    pthread_join(thread_id, NULL);
    thread_mutex_wait_signal_2.unlock();
    EXPECT_EQ(3, a);
}


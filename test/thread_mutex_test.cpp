#include "../include/thread_mutex.hpp"
#include "gtest/gtest.h"

long result = 0;
multi_thread::ThreadMutex thread_mutex;

void *add_thread(void *args) {
    for (int i = 0; i < 10000; i++) {
        thread_mutex.lock();
        result += 1;
        thread_mutex.unlock();
    }
    pthread_exit(NULL);
}

TEST(ThreadMutexTest, thread_mutex_lock_and_unlock) {
    pthread_t thread[50];
    for (int i = 0; i < 50; i++) {
        int return_value = pthread_create(&thread[i], NULL, add_thread, NULL);
        if (return_value != 0) {
            cout << "create thread error." << endl;
        }
    }

    for (int i = 0; i < 50; i++) {
        pthread_join(thread[i], NULL);
    }

    EXPECT_EQ(500000, result);
}
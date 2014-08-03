#include "../include/thread_pool.hpp"
#include "gtest/gtest.h"


class TestJob: public ::multi_thread::Job {
public:
    TestJob() {
        _value = -1;
    }
    ~TestJob() {}
    void run(void* jobdata) {
        int value = *static_cast<int*>(jobdata);
        //cout << "TestJob" << value << endl;
        _value = value;
    }
    int get_value() const {
        return _value;
    }

private:
    int _value;

};

int thread_pool_test_A(int num) {
    multi_thread::ThreadPool thread_pool(num);
    thread_pool.set_max_thread_idle(60);
    vector<TestJob*> test_job_list;
    int sum = 0;
    for (int i = 0; i < num; i++) {
        TestJob *job = new TestJob;
        int *a = new int;
        *a = i+1;
        thread_pool.run(job, a);
        test_job_list.push_back(job);
    }

    int count_down = 5;
    while (!test_job_list.empty() && count_down >= 0) {
        for (int i = 0; i < test_job_list.size(); i++) {
            if (test_job_list[i]->get_value() != -1) {
                sum += test_job_list[i]->get_value();
                delete test_job_list[i];
                test_job_list.erase(test_job_list.begin()+i);
                i -= 1;
            } else {
                continue;
            }
        }
        if (!test_job_list.empty()) {
            sleep(2);
            count_down -= 1;
        }
    }
    //ThreadPool.destroy_all_thread();
    return sum;
}

int thread_pool_test_B(int num) {
    multi_thread::ThreadPool thread_pool;
    thread_pool.set_max_thread_idle(num);
    vector<TestJob*> test_job_list;
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        TestJob *job = new TestJob;
        int *a = new int;
        *a = i+1;
        thread_pool.run(job, a);
        test_job_list.push_back(job);
    }

    int count_down = 5;
    while (!test_job_list.empty() && count_down >= 0) {
        for (int i = 0; i < test_job_list.size(); i++) {
            if (test_job_list[i]->get_value() != -1) {
                sum += test_job_list[i]->get_value();
                delete test_job_list[i];
                test_job_list.erase(test_job_list.begin()+i);
                i -= 1;
            } else {
                continue;
            }
        }
        if (!test_job_list.empty()) {
            sleep(2);
            count_down -= 1;
        }
    }
    //ThreadPool.destroy_all_thread();
    return sum;
}

TEST(ThreadPoolTest, Thread_pool_test_static_max_dynamic_input) {
    EXPECT_EQ(0, thread_pool_test_A(0));
    EXPECT_EQ(15, thread_pool_test_A(5));
    EXPECT_EQ(210, thread_pool_test_A(20));
    EXPECT_EQ(1275, thread_pool_test_A(50));
    EXPECT_EQ(5050, thread_pool_test_A(100));
    EXPECT_EQ(45150, thread_pool_test_A(300));
}

TEST(ThreadPoolTest, thread_pool_test_static_input_dynamic_max) {
    EXPECT_EQ(5050, thread_pool_test_B(0));
    EXPECT_EQ(5050, thread_pool_test_B(10));
    EXPECT_EQ(5050, thread_pool_test_B(20));
    EXPECT_EQ(5050, thread_pool_test_B(50));
    EXPECT_EQ(5050, thread_pool_test_B(100));
    EXPECT_EQ(5050, thread_pool_test_B(300));
}

#include <vector>
#include <iostream>
#include <thread>
#include <functional>
#include <queue>
#include "../basic_semaphore.h"

using namespace std;

template <int _t_pool_size>
class thread_pool {
    using t_thread_pool = thread_pool<_t_pool_size>;
    using t_pool_semaphore = basic_semaphore<_t_pool_size>;
    using t_binary_semaphore = basic_semaphore<1>;
    using t_thread_function = function<void(void*)>;

public:
    thread_pool() : m_flow_control_sem(0),
                    m_execute_signal_sem(0),
                    m_pool_lock_sem(1),
                    m_task_runner(thread_pool_routine, ref(*this)),
                    m_is_thread_running(true) {}

    ~thread_pool() { 
        join();    
    }

    void initialize() {
        m_pool_lock_sem.wait();
        for (int i = 0; i < _t_pool_size; i++)  {
            m_thread_vec.emplace_back(this);
        }
        m_pool_lock_sem.signal();
    }

    void run_task_async(t_thread_function task, void* arg = nullptr) {
        m_pool_lock_sem.wait();
        
        // 이미 쓰레드풀이 join() 되버린 경우 작업을 더이상 못하므로 나간다.
        if (m_is_thread_running) {
            m_pool_lock_sem.signal();
            return;
        }

        m_waiting_task.push(move(task));
        m_pool_lock_sem.signal();
        m_execute_signal_sem.signal();
    }

    void join() {
        cout << "thread pool : join\n";
        
        m_pool_lock_sem.wait();
        cout << "thread pool : thread running : false\n";
        m_is_thread_running = false;
        m_pool_lock_sem.signal();
        
        while (!m_is_thread_end)
            m_execute_signal_sem.signal();
        m_task_runner.join();
    }

private:
    static void thread_pool_routine(thread_pool& this_) {
        cout << "thread pool : start routine\n";

        do {
            cout << "thread pool : wait for task\n";
            this_.m_execute_signal_sem.wait();
            cout << "thread pool : get execute signal\n";
            this_.m_pool_lock_sem.wait();
            cout << "thread pool : check tasks...\n";
            while (!this_.m_waiting_task.empty()) {
                pool_task&& task = move(this_.m_waiting_task.front());
                this_.m_waiting_task.pop();   
                this_.m_pool_lock_sem.signal();
                this_.m_flow_control_sem.wait();
                for (int i = 0; i < _t_pool_size; i++) {
                    if (!this_.m_thread_vec[i].is_task_running()) {
                        this_.m_thread_vec[i].execute(move(task));
                    }
                }
            }

            this_.m_pool_lock_sem.signal();
        } while (this_.m_is_thread_running);
        
        cout << "thread pool : release child threads\n";
        for (size_t i = 0; i < this_.m_thread_vec.size(); i++) {
            this_.m_thread_vec[i].join();
        }
        this_.m_is_thread_end = true;
    }
private:
    struct pool_task {
        t_thread_function m_fn;
        bool m_executed;
        void* m_arg;

        void execute() {
            m_fn(m_arg);
            m_executed = true;
        }
    };


    struct pool_thread {
        t_thread_pool* m_pool;
        t_binary_semaphore m_execute_signal_sem; 
        t_binary_semaphore m_variable_lock_sem;
        thread m_thread;
        pool_task m_task; 
        bool m_is_task_running;
        bool m_is_thread_running;
        bool m_is_thread_end;

        pool_thread(t_thread_pool* pool) : 
            m_execute_signal_sem(0),
            m_variable_lock_sem(1),
            m_thread(thread_routine, ref(*this)),
            m_is_task_running(false),
            m_is_thread_running(true),
            m_is_thread_end(false) {}

        ~pool_thread() noexcept {
            join();
        }

        void join() {
            m_variable_lock_sem.wait();
            m_is_thread_running = true;
            m_variable_lock_sem.signal();

            while (!m_is_thread_end)
                m_execute_signal_sem.signal();

            m_thread.join();
        }

        bool is_task_running() {
            m_variable_lock_sem.wait();
            bool running = m_is_task_running;
            m_variable_lock_sem.signal();
            return running;
        }

        void execute(pool_task&& task) {
            m_variable_lock_sem.wait();
            m_task = move(task);
            m_variable_lock_sem.signal();
            m_execute_signal_sem.signal();
        }

        static void thread_routine(pool_thread& this_) {

            cout << "thread : wait for running\n";
            while (this_.m_is_thread_running) {
                this_.m_execute_signal_sem.wait();
                this_.m_variable_lock_sem.wait();
                this_.m_is_task_running = true;

                if (!this_.m_task.m_executed)
                    this_.m_task.execute();

                this_.m_is_task_running = false;
                this_.m_variable_lock_sem.signal();
                this_.m_pool->m_flow_control_sem.signal();
            }

            this_.m_is_thread_end = true;
        }
        

    };
private:
    vector<pool_thread> m_thread_vec;
    queue<pool_task> m_waiting_task;
    t_pool_semaphore m_flow_control_sem;
    t_binary_semaphore m_execute_signal_sem;    // 작업 요청을 하면 
    t_binary_semaphore m_pool_lock_sem;         // 멀티 쓰레딩 환경에서 안전하게 쓰레드풀을 사용할 수 있도록 하기위한 뮤텍스
    thread m_task_runner;
    bool m_is_thread_running;
    bool m_is_thread_end;
};

int main() {
    thread_pool<8> pool;
    pool.initialize();
    return 0;
}

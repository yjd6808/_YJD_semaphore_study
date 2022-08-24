/*
 * 내가 만든 세마포어랑 상상만으로 만들어본 쓰레드 풀
 * 쓰레드 여러개 미리 만들어놓고 함수 실행할 때 놀고있는 쓰레드를 깨워서 작업을 처리하도록 하는 걸로 대충 이해하고 있어서 만들어봤다.
 * 처음 만들어본거라 구조도 많이 이상하고 코드도 너무 더럽다.
 * 분석자체가 불가능할 지경임
 * 이 코드는 걍 폐기 처분하는걸로 하고 유튜브에서 쓰레드 풀 관련 영상을 보고 다시 한번 구현해봐야겠다.
 *
 * std::mutex가 복사/이동 생성자 호출이 불가능해서 std::vector emplace back 호출을 할 수가 없음;
 * 그래서 std::array로 만들었다. std::array도 생성자로 전부 초기화하는 것도 빡셈; (https://stackoverflow.com/questions/70931587/how-to-initialize-stdarray-member-in-constructor-initialization-list-when-the)
 * 다른 방식 생각나면 그때 쓰레드 수 제한하는건 구현하는 걸로
 */

#include <array>
#include <iostream>
#include <thread>
#include <functional>
#include <queue>
#include <chrono>
#include "../basic_semaphore.h"

using namespace std;
using namespace std::chrono_literals;

template <int _t_pool_size>
class thread_pool {
    using t_thread_pool = thread_pool<_t_pool_size>;
    using t_pool_semaphore = basic_semaphore<_t_pool_size>;
    using t_binary_semaphore = basic_semaphore<1>;
    using t_thread_function = function<void(void*)>;

    struct pool_task {
        t_thread_function m_fn;
        bool m_executed;
        void* m_arg;

        pool_task(): m_fn(nullptr), m_arg(nullptr), m_executed(false) {}
        pool_task(t_thread_function fn, void* arg) : m_fn(fn), m_arg(arg), m_executed(false) {}

        void execute() {
            if (m_fn) {
                m_fn(m_arg);
                m_executed = true;
            }
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

        pool_thread(const pool_thread& other) = default;
        pool_thread(pool_thread&& other) = default;
        pool_thread(t_thread_pool* pool) :
            m_pool(pool),
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

            if (!m_is_thread_running) {
                m_variable_lock_sem.signal();
                return;
            }

            m_is_thread_running = false;
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

            cout << "\tthread : wait for running\n";
            while (this_.m_is_thread_running) {
                this_.m_execute_signal_sem.wait();

                this_.m_variable_lock_sem.wait();
                this_.m_is_task_running = true;
                this_.m_variable_lock_sem.signal();

                if (!this_.m_task.m_executed) {
                    this_.m_task.execute();
                }

                if (this_.m_task.m_executed) {
                    this_thread::sleep_for(1000ms);
                }


                this_.m_variable_lock_sem.wait();
                this_.m_is_task_running = false;
                this_.m_variable_lock_sem.signal();
                
                this_.m_pool->m_flow_control_sem.signal();
            }

            cout << "\tthread : end...\n";
            this_.m_is_thread_end = true;
        }


    };
public:
    thread_pool() :
        m_thread_arr{ this, this, this, this, this },
		m_flow_control_sem(_t_pool_size),
        m_execute_signal_sem(0),
        m_pool_lock_sem(1),
        m_task_runner(thread_pool_routine, ref(*this)),
        m_is_thread_running(true) {}

    ~thread_pool() {
        join();
    }

    void run_task_async(t_thread_function task, void* arg = nullptr) {
        m_pool_lock_sem.wait();

        // 이미 쓰레드풀이 join() 되버린 경우 작업을 더이상 못하므로 나간다.
        if (!m_is_thread_running) {
            m_pool_lock_sem.signal();
            return;
        }

        m_waiting_task.emplace(move(task), arg);
        m_pool_lock_sem.signal();
        m_execute_signal_sem.signal();
    }

    void join() {
        cout << "thread pool : join\n";

        m_pool_lock_sem.wait();

        // 이미 쓰레드풀이 join() 되버린 경우 작업을 더이상 못하므로 나간다.
        if (!m_is_thread_running) {
            m_pool_lock_sem.signal();
            return;
        }

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
                pool_task task = move(this_.m_waiting_task.front());
                this_.m_waiting_task.pop();
                this_.m_pool_lock_sem.signal();
                this_.m_flow_control_sem.wait();
                for (int i = 0; i < _t_pool_size; i++) {
                    if (!this_.m_thread_arr[i].is_task_running()) {
                    	this_.m_thread_arr[i].execute(move(task));
                        break;
                    }
                }
            }

            this_.m_pool_lock_sem.signal();
        } while (this_.m_is_thread_running);

        cout << "thread pool : release child threads\n";
        for (size_t i = 0; i < this_.m_thread_arr.size(); i++) {
            this_.m_thread_arr[i].join();
        }
        this_.m_is_thread_end = true;
    }
  
private:
    array<pool_thread, _t_pool_size> m_thread_arr;
    queue<pool_task> m_waiting_task;
    t_pool_semaphore m_flow_control_sem;
    t_binary_semaphore m_execute_signal_sem;    // 작업 요청을 하면 
    t_binary_semaphore m_pool_lock_sem;         // 멀티 쓰레딩 환경에서 안전하게 쓰레드풀을 사용할 수 있도록 하기위한 뮤텍스
    thread m_task_runner;
    bool m_is_thread_running;
    bool m_is_thread_end;
};

int main() {
    thread_pool<5> pool;

    for (int i = 0; i < 100; i++)
        pool.run_task_async([](void* args) { cout << "aaaa\n"; });

    pool.join();
    return 0;
}


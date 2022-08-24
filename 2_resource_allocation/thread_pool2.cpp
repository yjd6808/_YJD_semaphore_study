// 세마포어로만 쓰레드풀 만들기 2번째 버전

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <atomic>
#include <queue>
#include <thread>
#include <tuple>

#include "../basic_semaphore.h"

using namespace std;
using namespace std::chrono_literals;

struct task {
    using t_function = function<void(void*)>;

    t_function m_fn = nullptr;
    void* m_arg = nullptr;

    void execute() {
        if (m_fn) m_fn(m_arg);
    }
};

template <typename pool> class pool_thread {
    using t_pool_thread = pool_thread<pool>;
    using task_function = function<void(void*)>;
public:
    pool_thread(pool* parent) : 
        m_thread([this] { thread_routine(); }),
        m_pool(parent),
        m_running(true),
        m_waiter(0)
    {
    }

    ~pool_thread() {
    }

    void run(task&& task) { 
        {
            sem_guard<bin_semaphore> guard(m_lock);
            m_pending_task = std::move(task);
        }
        m_has_task = true;
        m_waiter.signal();
    } 

    void join() {
        m_running = false;
        m_waiter.signal();
        m_thread.join();
    }
     
private:
    void thread_routine() {
        for (;;) {
            m_waiter.wait();
            if (!m_running && !m_has_task) break;

            task executable_task;
            {
                sem_guard<bin_semaphore> guard(m_lock);
                executable_task = std::move(m_pending_task);
            }
            executable_task.execute();
            m_has_task = false;
            m_pool->release(this);
            if (!m_running) break;
        }
    }
private:
    std::thread m_thread;
    pool* m_pool;
    bool m_running;
    bool m_has_task;
    bin_semaphore m_waiter;
    bin_semaphore m_lock;
    task m_pending_task;
};



template <int pool_size>
class thread_pool {
    using t_pool = thread_pool<pool_size>;
    using t_pool_sem = basic_semaphore<pool_size>;
    using t_pool_thread = pool_thread<thread_pool<pool_size>>;
public:
    enum class state {
        running,                         // 쓰레드 풀이 생성되자마자 running 상태
        join_wait_pending,               // 작업 큐에 있는 작업들이 모두 처리된 후에 join 되도록 함 
        join_wait_ignore_pending,        // 현재 진행중인 작업들만 작업을 수행하고 작업 큐에있는 작업들은 모두 폐기처리함
        joined
    };
public:
    thread_pool() :
        m_space_sem(5),
        m_pool_lock(1),
        m_signal_sem(0),
        m_thread([this] { thread_routine(); })
    {
        set_state(state::running);

        for (int i = 0; i < pool_size; i++)
            m_waiting_queue.push(new t_pool_thread(this));
    }

    ~thread_pool() {
        try {
            join(false);
        } catch (const std::string& msg) {
            cout << "쓰레드 풀이 강제로 종료되었습니다.\n";
        } 
    }

    void enqueue(task::t_function&& fn, void* arg = nullptr) {
        if (get_state() >= state::join_wait_pending)
            throw std::string{"join 중에는 작업을 넣을 수 없습니다."};

        {
            sem_guard<bin_semaphore> guard(m_pool_lock);
            m_task_queue.emplace(std::move(fn), arg);
        }
        
        m_signal_sem.signal();
    }

    void release(t_pool_thread* finished_thread) {
        {
            sem_guard<bin_semaphore> guard(m_pool_lock);
            auto find_it = std::find(m_running_queue.begin(), m_running_queue.end(), finished_thread);
            if (find_it != m_running_queue.end())
                m_running_queue.erase(find_it);
            m_waiting_queue.push(finished_thread);
        }

        m_space_sem.signal();
    }

    void set_state(state st) {
        m_state = int(st);
    }

    state get_state() {
        return state(m_state.load());
    }

    void join(bool wait_all = false) {
        if (get_state() >= state::join_wait_pending) {
            throw std::string{"join 중이거나 이미 join 되었습니다."};
        }

        set_state(wait_all ? 
                state::join_wait_pending : 
                state::join_wait_ignore_pending);
        m_signal_sem.signal();
        m_thread.join();
    }

    size_t get_enqueued_task_count() {
        size_t count = m_has_pending_task ? 1 : 0;

        {
            sem_guard<bin_semaphore> guard(m_pool_lock);
            count += m_task_queue.size();
        }

        return count;
    }

    size_t get_running_task_count() {
        sem_guard<bin_semaphore> guard(m_pool_lock);
        return m_running_queue.size();
    }
private:
    void thread_routine() {
        bool joined = false;

        for (;;) {
            m_signal_sem.wait();
            joined = flush_tasks();
            if (joined) break;
        }

        // 현재 돌아가고 있는 작업들이 모두 완료될 때까지 확인함
        while (get_running_task_count() > 0) {
            this_thread::sleep_for(1ms);
        }
        
        while (!m_waiting_queue.empty()) {
            t_pool_thread* th = m_waiting_queue.front();
            th->join();
            m_waiting_queue.pop();
            delete th; 
        }

        set_state(state::joined);
    }

    bool flush_tasks() {
        t_pool_thread* free_thread = nullptr;
        task cur_task;

        for (;;) {


            {
                sem_guard<bin_semaphore> guard(m_pool_lock);
                state st = get_state();
                if (st >= state::join_wait_ignore_pending) return true;

                if (!m_task_queue.empty()) {
                    cur_task = m_task_queue.front();
                    m_task_queue.pop();
                } else if (st == state::join_wait_pending) {
                    return true;
                }
            }

            m_has_pending_task = true;
            m_space_sem.wait();

            state st = get_state();

            if (st >= state::join_wait_ignore_pending) {
                m_has_pending_task = false;
                return true;
            }

            {
                sem_guard<bin_semaphore> guard(m_pool_lock);
                free_thread = m_waiting_queue.front(); 
                m_waiting_queue.pop();

                m_running_queue.push_back(free_thread);
                free_thread->run(std::move(cur_task));
                m_has_pending_task = false;
            }
        }

        return false;
    }
private:
    queue<t_pool_thread*>     m_waiting_queue;
    vector<t_pool_thread*>    m_running_queue;
    queue<task>               m_task_queue;

    t_pool_sem         m_space_sem;
    bin_semaphore      m_pool_lock;
    bin_semaphore      m_signal_sem;

    atomic<int>        m_state;
    bool               m_has_pending_task;

    thread             m_thread;
};

int main() {
    thread_pool<5> pool;
    int a;

    while (std::cin >> a && a != -1) {
        pool.enqueue([](void* arg) {
            cout << rand() << "\n";
            this_thread::sleep_for(chrono::milliseconds((rand() % 6 * 300)));
        });
    }

    try {
        pool.join();
    } catch (const std::string& str) {
        int a = rand();
        cout << a << str << "\n";
        return 1;
    }
    return 0;
}



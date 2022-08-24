/*
 * 작성자 : 윤정도 가장 기본적인 형태의 세마포어 구현
 */
#pragma once

#include <mutex>

// _t_count : 최대로 크리티컬 섹션이 진입할 수 있는 쓰레드 수 
template <int _t_count> 
class basic_semaphore {
public:
    basic_semaphore(int use_count = _t_count) : m_usable_count(use_count) {}
public:
    // acquire(), lock()에 대응하는 함수 
    // 잠금을 획득할 수 있을때가지 대기한다.
    void wait() {
        while (true) {
            std::lock_guard<std::mutex> lg(m_mtx);
            if (m_usable_count > 0) {
                m_usable_count--;
                break;
            }
        }
    }

    // release(), unlock()에 대응하는 함수
    void signal() {
        std::lock_guard<std::mutex> lg(m_mtx);
        if (m_usable_count < _t_count)
            m_usable_count++;
    }

    bool locked() {
        std::lock_guard<std::mutex> lg(m_mtx);
        return m_usable_count == 0;
    }

private:
    int m_usable_count;     // 현재 잠금을 획득가능한 쓰레드 수
    std::mutex m_mtx;       // m_usable_count에 대해서 원자적 연산을 수행해주기 위한 뮤텍스
};


using bin_semaphore = basic_semaphore<1>;

template <typename _t_sem>
struct sem_guard {
    sem_guard(_t_sem& sem): m_sem(&sem) { sem.wait(); }
    ~sem_guard() { m_sem->signal(); }

   _t_sem* m_sem;
};

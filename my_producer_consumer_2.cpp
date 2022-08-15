/*
 * 작성자 : 윤정도
 * 가장 기본적인 형태의 세마포어 구현
 *
 * 단일 생산자-소비자 패턴 (1)
 *
 * 문제 : 생산자 쓰레드에서 buffer에 데이터를 넣어주면 consumer 쓰레드에서
 *        데이터가 들어오면 가져다가 쓸 수 있도록 코드를 작성해봐라.
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <semaphore>
#include <string.h>

#include "basic_semaphore.h"

using namespace std::chrono_literals;

const int buf_size = 5;

int seq_number = 0;
char buffer[buf_size]{};            // 생산된 데이터가 저장될 버퍼

int write_pos = 0;                  // producer 쓰레드가 생산한 데이터가 들어갈 위치
int read_pos = 0;                   // consumer 쓰레드가 생산된 데이터를 읽을 위치

basic_semaphore<5> consume_sem(0);  // 초기 세마포어는 0으로 초기화하도록 한다. 
                                    // 데이터가 준비되면 signal()을 줘서 1만큼 올려준다.
                                    // 이때 wait() 함수로 대기중인 쓰레드에서 반복문을 탈출하여
                                    // 크리티컬 섹션에 진입할 수 있게된다.

basic_semaphore<5> produce_sem(buf_size);   // 생산 가능한 데이터 수만큼으로 초기화 한다.
basic_semaphore<1> guard_sem(1);            // 여러 쓰레드가 한번에 데이터를 생산해버리기 때문에 생산/소비 과정 자체는
                                            // 원자적으로 수행할 수 있도록 해줘야한다.

void producer() {
    produce_sem.wait();
    guard_sem.wait();

    buffer[write_pos] = ++seq_number;
    std::cout << "쓰기 위치 : " << write_pos <<  " / " << seq_number << " 데이터가 생산되었습니다.\n";
    int next_write_pos = ++write_pos;
    write_pos = next_write_pos % buf_size;

    guard_sem.signal();
    consume_sem.signal();
}

void consumer() {
    consume_sem.wait();
    guard_sem.wait();

    int read_data = buffer[read_pos];
    std::cout << "읽은 위치 : " << read_pos <<  " / " << read_data << " 데이터가 소비되었습니다.\n";
    int next_read_pos = ++read_pos;
    read_pos = next_read_pos % buf_size;

    guard_sem.signal();
    produce_sem.signal();
}

int loop_count = 30;

void reset_data(int _loop_count) {
    loop_count = _loop_count;
    read_pos = 0;
    write_pos = 0;
    seq_number = 0;
    char clr_data = 0;
    memcpy(buffer, &clr_data, sizeof(char) * buf_size);
}

int main() {
    
    loop_count = 30;

    // 1:1 생산자/소비자 구조에서는 이상없이 잘 동작한다.
    std::cout << "[[ 생산자 1 : 소비자 1 / 30회 루프]]\n";
    while (loop_count--) {
        std::thread th2(consumer);
        std::thread th1(producer);

        th2.join();
        th1.join();
    }

    reset_data(10);
    // 생산자나 여러개가 되어버린 경우 데이터를 덮어쓰거나
    // 생산과정이 원자적으로 수행되지 않아서 잘못된 결과를 얻를 수 있다.
    std::cout << "\n[[ 생산자 多(5) : 소비자 多(5) / 10회 루프]]\n";
    while (loop_count--) {
        std::thread th6(consumer);
        std::thread th7(consumer);
        std::thread th8(consumer);
        std::thread th9(consumer);
        std::thread th10(consumer);

        std::thread th1(producer);
        std::thread th2(producer);
        std::thread th3(producer);
        std::thread th4(producer);
        std::thread th5(producer);

        

        th1.join();
        th2.join();
        th3.join();
        th4.join();
        th5.join();
        th6.join();
        th7.join();
        th8.join();
        th9.join();
        th10.join();

    }
}

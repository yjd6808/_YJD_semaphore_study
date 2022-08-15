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

#include "basic_semaphore.h"

using namespace std::chrono_literals;

const int buf_size = 5;

int seq_number = 0;
char buffer[buf_size]{};            // 생산된 데이터가 저장될 버퍼

int write_pos = 0;                  // producer 쓰레드가 생산한 데이터가 들어갈 위치
int read_pos = 0;                   // consumer 쓰레드가 생산된 데이터를 읽을 위치

binary_semaphore sem(0);            // 초기 세마포어는 0으로 초기화하도록 한다. 
                                    // 데이터가 준비되면 signal()을 줘서 1만큼 올려준다.
                                    // 이때 wait() 함수로 대기중인 쓰레드에서 반복문을 탈출하여
                                    // 크리티컬 섹션에 진입할 수 있게된다.

void producer() {
    buffer[write_pos] = ++seq_number;
    std::cout << "쓰기 위치 : " << write_pos <<  " / " << seq_number << " 데이터가 생산되었습니다.\n";
    int next_write_pos = ++write_pos;
    write_pos = next_write_pos % buf_size;
    sem.signal();
}

void consumer() {
    sem.wait();
    // 이하 크리티컬 섹션
    //
    // 바이너리 세마포어 이므로 하나의 쓰레드만 wait() 
    // 함수내부의 반복문을 탈출할 수 있게된다.
    int read_data = buffer[read_pos];
    std::cout << "읽은 위치 : " << read_pos <<  " / " << read_data << " 데이터가 소비되었습니다.\n";
    int next_read_pos = ++read_pos;
    read_pos = next_read_pos % buf_size;
}

int main() {
    int loop_count = 30;

    while (loop_count--) {
        std::thread th2(consumer);
        std::thread th1(producer);

        th2.join();
        th1.join();
    }
    
    return 0;
}

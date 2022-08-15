md 파일 작성 연습하는김에 적음 <br>
블로그에 더 깔금하게 정리하자.

## 세마포어

### <b>basic_semaphore.h</b>  
학습 후 가장 단순하게 구현해본 세마포어

<br>

### <b>어따 쓸까?</b>

 - [생산자 소비자 문제](#producer_consumer)
 - 동시에 작업가능한 자원의 수 제한
 - 쓰레드간 작업의 우선순위 결정
 - 뮤텍스 (바이너리 세마포어)

<br>

---


# 1. 생산자 소비자 문제 <a name="producer_consumer"></a>

### 생산자 소비자 버전 1 (my_producer_consumer_1.cpp)

<br>

#### <b>실행 결과</b>  
 ```
 $ make
g++ -Wall -std=c++20 -o my_producer_consumer_1.elf my_producer_consumer_1.cpp

 $ ./my_producer_consumer_1.elf 
쓰기 위치 : 0 / 1 데이터가 생산되었습니다.
읽은 위치 : 0 / 1 데이터가 소비되었습니다.
쓰기 위치 : 1 / 2 데이터가 생산되었습니다.
읽은 위치 : 1 / 2 데이터가 소비되었습니다.
쓰기 위치 : 2 / 3 데이터가 생산되었습니다.
읽은 위치 : 2 / 3 데이터가 소비되었습니다.
쓰기 위치 : 3 / 4 데이터가 생산되었습니다.
읽은 위치 : 3 / 4 데이터가 소비되었습니다.
쓰기 위치 : 4 / 5 데이터가 생산되었습니다.
읽은 위치 : 4 / 5 데이터가 소비되었습니다.
쓰기 위치 : 0 / 6 데이터가 생산되었습니다.
읽은 위치 : 0 / 6 데이터가 소비되었습니다.
쓰기 위치 : 1 / 7 데이터가 생산되었습니다.
읽은 위치 : 1 / 7 데이터가 소비되었습니다.
쓰기 위치 : 2 / 8 데이터가 생산되었습니다.
읽은 위치 : 2 / 8 데이터가 소비되었습니다.
쓰기 위치 : 3 / 9 데이터가 생산되었습니다.
읽은 위치 : 3 / 9 데이터가 소비되었습니다.
쓰기 위치 : 4 / 10 데이터가 생산되었습니다.
읽은 위치 : 4 / 10 데이터가 소비되었습니다.
쓰기 위치 : 0 / 11 데이터가 생산되었습니다.
읽은 위치 : 0 / 11 데이터가 소비되었습니다.
쓰기 위치 : 1 / 12 데이터가 생산되었습니다.
읽은 위치 : 1 / 12 데이터가 소비되었습니다.
쓰기 위치 : 2 / 13 데이터가 생산되었습니다.
읽은 위치 : 2 / 13 데이터가 소비되었습니다.
쓰기 위치 : 3 / 14 데이터가 생산되었습니다.
읽은 위치 : 3 / 14 데이터가 소비되었습니다.
쓰기 위치 : 4 / 15 데이터가 생산되었습니다.
읽은 위치 : 4 / 15 데이터가 소비되었습니다.
쓰기 위치 : 0 / 16 데이터가 생산되었습니다.
읽은 위치 : 0 / 16 데이터가 소비되었습니다.
쓰기 위치 : 1 / 17 데이터가 생산되었습니다.
읽은 위치 : 1 / 17 데이터가 소비되었습니다.
쓰기 위치 : 2 / 18 데이터가 생산되었습니다.
읽은 위치 : 2 / 18 데이터가 소비되었습니다.
쓰기 위치 : 3 / 19 데이터가 생산되었습니다.
읽은 위치 : 3 / 19 데이터가 소비되었습니다.
쓰기 위치 : 4 / 20 데이터가 생산되었습니다.
읽은 위치 : 4 / 20 데이터가 소비되었습니다.
쓰기 위치 : 0 / 21 데이터가 생산되었습니다.
읽은 위치 : 0 / 21 데이터가 소비되었습니다.
쓰기 위치 : 1 / 22 데이터가 생산되었습니다.
읽은 위치 : 1 / 22 데이터가 소비되었습니다.
쓰기 위치 : 2 / 23 데이터가 생산되었습니다.
읽은 위치 : 2 / 23 데이터가 소비되었습니다.
쓰기 위치 : 3 / 24 데이터가 생산되었습니다.
읽은 위치 : 3 / 24 데이터가 소비되었습니다.
쓰기 위치 : 4 / 25 데이터가 생산되었습니다.
읽은 위치 : 4 / 25 데이터가 소비되었습니다.
쓰기 위치 : 0 / 26 데이터가 생산되었습니다.
읽은 위치 : 0 / 26 데이터가 소비되었습니다.
쓰기 위치 : 1 / 27 데이터가 생산되었습니다.
읽은 위치 : 1 / 27 데이터가 소비되었습니다.
쓰기 위치 : 2 / 28 데이터가 생산되었습니다.
읽은 위치 : 2 / 28 데이터가 소비되었습니다.
쓰기 위치 : 3 / 29 데이터가 생산되었습니다.
읽은 위치 : 3 / 29 데이터가 소비되었습니다.
쓰기 위치 : 4 / 30 데이터가 생산되었습니다.
읽은 위치 : 4 / 30 데이터가 소비되었습니다.
 
 ```
#### <b>문제점</b>   
 - 데이터 생성이 소비하는 속도보다 훨씬 빠를 경우 기존 버퍼의 데이터를 덮어 써버릴 수 있다.
 - 하나의 생산자, 소비자만 사용가능하다.

#### <b>어떻게 개선하면 좋을까?</b>  
 - 세마포어가 2개더 필요하다.  
   1. 소비자가 데이터가 존재해야 가져올 수 있으니까 마찬가지로 생산자도 데이터를 생산할 수 있는 상태여야 생산 가능하도록 제어해줄 세마포어 
   2. 여러 쓰레드에서 생산/소비를 진행하기 때문에 데이터 생성/소비가 원자 단위로 수행될 수 있도록 해줄 세마포어 혹은 뮤텍스


내일 개선된 코드를 작성해보자.  
오늘은 요까지
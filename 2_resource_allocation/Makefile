 # 작성자 : 윤정도
 # 각 cpp 파일을 컴파일해서 곧바로 elf 확장자의 실행파일을 만들어줍니다.
CPPS := $(wildcard *.cpp)
ELFS := $(CPPS:.cpp=.elf)

CPPFLAGS := -Wall -g -std=c++20


.PHONY: all
all: $(ELFS)


$(ELFS): %.elf: %.cpp
	$(CXX) $(CPPFLAGS) -o $@ -I.. $<

clean:
	rm -f *.elf

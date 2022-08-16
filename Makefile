 # 작성자 : 윤정도
 # 하위 디렉토리들에 대해서 한번에 메이크 작업을 수행합니다. 


SUB_DIRS := 1_producer_consumer 2_resource_allocation

all: $(SUB_DIRS)

$(SUB_DIRS):
	$(info aaa)
	cd $@; $(MAKE)	$(MAKECMDGOALS)

.PHONY: $(SUB_DIRS)

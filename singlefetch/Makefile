SOURCES := utils.c disasm.c emulator.c riscv.c pipeline.c cache.c
HEADERS := types.h utils.h riscv.h pipeline.h stage_helpers.h cache.h
PWD := $(shell pwd)
CUNIT := -L $(PWD)/CUnit-install/lib -I $(PWD)/CUnit-install/include -llibcunit
CFLAGS := -g -Wall -m64

all: riscv

riscv: $(SOURCES) $(HEADERS)
	gcc $(CFLAGS) -o $@ $(SOURCES)

test-utils: test_utils.c utils.c $(HEADERS)
	gcc $(CFLAGS) -DTESTING -o test-utils test_utils.c utils.c $(CUNIT)
	./test-utils
	rm -f test-utils

clean:
	rm -f riscv
	rm -f *.o *~
	rm -f test-utils
	rm -f code/part*/out/*.solution code/part*/out/*/*.solution
	rm -f code/part*/out/*.trace code/part*/out/*/*.trace

deepclean: clean
	rm -rf CUnit-install

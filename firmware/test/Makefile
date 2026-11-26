# Host unit tests (clang/gcc on the development machine, no hardware).
CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O1 -I../src -fsanitize=address,undefined
TESTS = test_nec test_ir_timing test_remote

all: $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; done

test_nec: test_nec.c ../src/nec.c ../src/nec.h
	$(CC) $(CFLAGS) -o $@ test_nec.c ../src/nec.c
test_ir_timing: test_ir_timing.c ../src/ir_timing.c ../src/ir_timing.h
	$(CC) $(CFLAGS) -o $@ test_ir_timing.c ../src/ir_timing.c
test_remote: test_remote.c ../src/remote.c ../src/remote.h
	$(CC) $(CFLAGS) -o $@ test_remote.c ../src/remote.c

clean:
	rm -f $(TESTS)
.PHONY: all clean

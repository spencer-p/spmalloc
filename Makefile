CFLAGS = -Wall -Werror -Wextra -fPIC
LDFLAGS = -shared
ifdef DEBUG
	CFLAGS += -g -DDEBUG
else
	CFLAGS += -O2
endif

.PHONY: clean

spmalloc.so: malloc.o util.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: test.c
	$(CC) test.c -o test

clean:
	rm -f test *.o *.so

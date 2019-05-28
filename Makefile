CFLAGS += -g

.PHONY: clean

test: test.o malloc.o util.o

clean:
	rm -f test *.o

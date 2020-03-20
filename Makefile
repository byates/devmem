
DEVMEM_CFLAGS := -Wall -std=gnu99 -ggdb3 -O3

%.o: %.c
	$(CC) $(DEVMEM_CFLAGS) $(CFLAGS) -c -o $@ $<

devmem: devmem.o
	$(CC) $(LDFLAGS) -o $@ $(DEVMEM_CFLAGS) $^

.PHONY: default
default: all

all: clean devmem

clean:
	-rm -f devmem *.o


CC ?= cc
CFLAGS ?= -Os
CPPFLAGS += -pedantic -Wall -Wextra
LDFLAGS += -lxcb -lxcb-image -lxcb-shm

BINS=ffshot

all: $(BINS)

ffshot: ffshot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o ffshot $^

clean:
	rm -f $(BINS)

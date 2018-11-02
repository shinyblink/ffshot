CC ?= cc
CFLAGS ?= -Os
CPPFLAGS += -pedantic -Wall -Wextra
LDFLAGS += -lxcb -lxcb-image -lxcb-shm
DESTDIR ?= /usr/local

# don't touch, maybe.
BINS=ffshot

all: $(BINS)

ffshot: ffshot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o ffshot $^

install: $(BINS)
	install $(BINS) $(DESTDIR)/bin

clean:
	rm -f $(BINS)

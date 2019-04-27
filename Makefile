CC ?= cc
CFLAGS ?= -Os
CPPFLAGS += -pedantic -Wall -Wextra
LDFLAGS += -lxcb -lxcb-image -lxcb-shm

PREFIX ?= /usr/local
DESTDIR ?= /

# don't touch, maybe.
BINS=ffshot

all: $(BINS)

ffshot: ffshot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o ffshot $^

install: $(BINS)
	install $(BINS) $(DESTDIR)/$(PREFIX)/bin

uninstall:
	cd $(DESTDIR)/$(PREFIX)/bin && rm $(BINS)

clean:
	rm -f $(BINS)

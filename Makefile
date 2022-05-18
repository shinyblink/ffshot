CC ?= cc
CFLAGS ?= -Os
CPPFLAGS += -pedantic -Wall -Wextra
LDFLAGS += -lxcb -lxcb-image

PREFIX ?= /usr/local
DESTDIR ?= /

# don't touch, maybe.
BINS=ffshot

all: $(BINS)

ffshot: ffshot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o ffshot $^

install: $(BINS)
	install -d $(DESTDIR)/$(PREFIX)/bin
	install $(BINS) $(DESTDIR)/$(PREFIX)/bin

uninstall:
	rm -f $(DESTDIR)/$(PREFIX)/$(BINS)

clean:
	rm -f $(BINS)

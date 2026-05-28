CC      = gcc
CFLAGS  = $(shell pkg-config --cflags gtk4) -O2 -Wall -Wextra
LIBS    = $(shell pkg-config --libs gtk4)
TARGET  = ./build/typebox

$(TARGET): ./src/typebox.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)

PREFIX ?= /usr/local

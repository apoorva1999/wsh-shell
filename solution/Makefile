CC=gcc
CFLAGS-common = -std=c17 -Wall -Wextra -Werror -pedantic -std=gnu18 -fno-omit-frame-pointer
CFLAGS = $(CFLAGS-common) -O2
CFLAGS-dbg = $(CFLAGS-common) -Og -ggdb
LOGIN=apoorva
SUBMITPATH= ~cs537-1/handin/$(LOGIN)/p3

TARGET = wsh
SRC = $(TARGET).c


all: $(TARGET) $(TARGET)-dbg
.PHONY: all 

$(TARGET): $(SRC) $(TARGET).h
	$(CC) $(CFLAGS) $< -o $@

$(TARGET)-dbg: $(SRC) $(TARGET).h
	$(CC) $(CFLAGS-dbg) $< -o $@


clean:
	rm -f $(TARGET) $(TARGET)-dbg

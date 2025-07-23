CC = clang
CFLAGS = -Iinclude -Wall -Wextra -std=c17 -Wno-unused-variable -I/usr/include/freetype2
LDFLAGS = -lX11 -lXft -lfreetype
SRC := $(shell find src -name '*.c')
OUT = mocha-shell

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS) -lm

clean:
	rm -f $(OUT)

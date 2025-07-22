CC = clang
CFLAGS = -Iinclude -Wall -Wextra -std=c17 -Wno-unused-variable
LDFLAGS = -lX11
SRC := $(shell find src -name '*.c')
OUT = mocha-wm

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

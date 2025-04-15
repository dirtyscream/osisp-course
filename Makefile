CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC_DIR = src
OUT = tcp_server

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lpthread
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -lpthread
endif
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lws2_32
endif

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OUT) $(SRC_DIR)/*.o

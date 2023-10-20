CC      =g++
CFLAGS  =-g -Wall -static
LDFLAGS =-L /home/wht/libzbd/lib/.libs/
LIBS    =-lzbd
OBJS    =zbuf.o common.o zalp.o zBuffer.o zController.o
TARGET  =zbuf

all: zbuf

zbuf: main.cpp src/common.cpp src/zalp.cpp src/zBuffer.cpp src/zController.cpp
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f ./*.o
	rm -f ./zbuf
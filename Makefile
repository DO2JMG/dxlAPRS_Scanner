CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DHAVE_LOG_H
LIBS    = -lpthread 
LDFLAGS = -g

OBJECTS = main.o

all:		udp

udp:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o scanner

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

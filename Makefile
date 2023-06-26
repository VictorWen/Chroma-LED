# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CFLAGS = -Wall -g -O0 -I libraries/boost_1_82_0
RM=rm -f

SRCS=src/main.cpp src/chroma.cpp src/chroma_script.cpp src/chromatic.cpp src/commands.cpp src/evaluator.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

# ****************************************************
# Targets needed to bring the executable up to date

all: chroma

chroma: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	$(RM) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
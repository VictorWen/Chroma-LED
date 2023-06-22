# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CFLAGS = -Wall -g -O0
 
# ****************************************************
# Targets needed to bring the executable up to date

default: chroma

chroma: src/*.cpp src/*.h
	$(CC) $(CFLAGS) -I ~/Documents/Libraries/boost_1_82_0 src/*.cpp src/*.h -o $@

chroma_script.o: src/chroma_script.cpp src/chroma_script.h
	$(CC) $(CFLAGS) -I ~/Documents/Libraries/boost_1_82_0 src/chroma_script.cpp src/chroma_scrip.h -o $@

chroma.o: src/chroma.cpp src/chroma.h
	$(CC) $(CFLAGS) -I ~/Documents/Libraries/boost_1_82_0 src/chroma.cpp src/chroma.h -o $@
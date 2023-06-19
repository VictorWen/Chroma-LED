# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CFLAGS = -Wall -g -O2
 
# ****************************************************
# Targets needed to bring the executable up to date

default: chroma

chroma: src/main.cpp src/chromatic.h src/chromatic.cpp
	$(CC) $(CFLAGS) -o $@ src/main.cpp src/chromatic.h src/chromatic.cpp

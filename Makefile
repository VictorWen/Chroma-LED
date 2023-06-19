# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CFLAGS = -Wall -g -O2
 
# ****************************************************
# Targets needed to bring the executable up to date

default: main

main: src/main.cpp
	$(CC) $(CFLAGS) -o chroma src/main.cpp

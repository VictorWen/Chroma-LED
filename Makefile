# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CFLAGS = -Wall -g -O0

DLCLIENT = wget
 
# ****************************************************
# Targets needed to bring the executable up to date

default: chroma

chroma: src/*.cpp src/*.h
	$(CC) $(CFLAGS) -I libraries/boost_1_82_0 src/*.cpp src/*.h -o $@

.PHONY: boost
boost: 
	cd libraries && $(DLCLIENT) https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz $(DLFLAGS) && tar -xvf boost_1_82_0.tar.gz && rm boost_1_82_0.tar.gz
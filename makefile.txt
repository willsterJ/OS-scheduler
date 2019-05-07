# Makefile 

# Variables to control Makefile operation

CXX = g++
CXXFLAGS = -Wall -g

# ****************************************************
# Targets needed to bring the executable up to date

sched: source.o classes.o
	$(CXX) $(CXXFLAGS) -o sched source.o classes.o


source.o: Source.cpp Classes.h
	$(CXX) $(CXXFLAGS) -c Source.cpp

classes.o: Classes.h
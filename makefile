# Makefile 

# Variables to control Makefile operation

CXX = g++
CXXFLAGS = -Wall -g

# ****************************************************
# Targets needed to bring the executable up to date
default: sched

sched: Source.o Classes.o
	$(CXX) $(CXXFLAGS) -o sched Source.o Classes.o

Source.o: Source.cpp Classes.h
	$(CXX) $(CXXFLAGS) -c Source.cpp

Classes.o: Classes.cpp Classes.h
	$(CXX) $(CXXFLAGS) -c Classes.cpp

clean:
	rm -rf *.o
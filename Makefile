CXX = g++
CXXFLAGS = -std=c++14 -Wall
SRC = main.cpp
EXECUTABLE = run

all: ${EXECUTABLE}

${EXECUTABLE}: ${SRC}
	$(CXX) $(CXXFLAGS) -o $@ $^
clean:
	rm -f ${EXECUTABLE}
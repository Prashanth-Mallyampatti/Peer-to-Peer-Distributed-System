# This is a GNU Makefile.

# Creating a static library
TARGET = p2p

# Libraries to use, objects to compile
SRCS = p2p.cpp
SRCS_FILES = $(foreach F, $(SRCS), ./$(F))
CXX_FLAGS = -pthread --std=c++11

# Make it all!
all : p2p.o
	g++ $(CXX_FLAGS) p2p.o -o $(TARGET)

p2p.o: p2p.cpp
	g++ $(CXX_FLAGS) p2p.cpp -c

# Standard make targets
clean :
	@rm -f *.o $(TARGET)

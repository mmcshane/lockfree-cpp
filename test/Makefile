CXX := g++
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(patsubst %.cpp,%.o,$(CPP_FILES))
LD_FLAGS := -pthread
CC_FLAGS := -DDEBUG -I../include -Wall -Werror

all: test

test: $(OBJ_FILES)
	$(CXX) $(LD_FLAGS) -o $@ $^

%.o: ./%.cpp
	$(CXX) $(CC_FLAGS) -c -o $@ $<


CXX      := x86_64-w64-mingw32-g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -Wno-cast-function-type -Wno-array-bounds
TARGET   := abyss_agent.exe

SRCS := main.cc $(shell find src -type f -name '*.cc' | sort)

.PHONY: all debug prod clean

all: prod

debug:
	$(CXX) $(CXXFLAGS) -D_DEBUG -O0 -g $(SRCS) -o $(TARGET)

prod:
	$(CXX) $(CXXFLAGS) -O2 $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

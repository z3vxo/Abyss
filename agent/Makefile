CXX      := x86_64-w64-mingw32-g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -Wno-cast-function-type -Wno-array-bounds
TARGET   := abyss_agent.exe

SRCS := main.cc \
        src/api/api.cc \
        src/core/reg.cc \
        src/core/checkin.cc \
        src/init/init.cc \
        src/init/config.cc \
        src/byte/byte.cc \
        src/core/core.cc \
        src/net/net.cc

.PHONY: all debug prod clean

all: prod

debug:
	$(CXX) $(CXXFLAGS) -D_DEBUG -O0 -g $(SRCS) -o $(TARGET)

prod:
	$(CXX) $(CXXFLAGS) -O2 $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

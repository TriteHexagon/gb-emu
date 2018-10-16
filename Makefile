BINARY_NAME := gb_emu

CXXFLAGS := -O2 -flto -Wall -Wextra

SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard src/*.h)

.PHONY: clean

$(BINARY_NAME): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(BINARY_NAME) `pkg-config --cflags --libs sdl2`

clean:
	$(RM) $(BINARY_NAME)

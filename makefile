# Compiler and basic flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3

# Final executable name
TARGET = kpc_solver

# Source files
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

# Main rule
all: $(TARGET)

# How to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# How to build the objects (.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleaning up compiled files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
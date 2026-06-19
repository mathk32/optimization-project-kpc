# Compiler and basic flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3

CPLEX = /opt/ibm/ILOG/CPLEX_Studio2211

INCLUDES = -I$(CPLEX)/cplex/include \
           -I$(CPLEX)/concert/include

LIBS = -L$(CPLEX)/cplex/lib/x86-64_linux/static_pic \
       -L$(CPLEX)/concert/lib/x86-64_linux/static_pic \
       -lilocplex -lconcert -lcplex -lm -lpthread
	   
# Final executable name
TARGET = kpc_solver

# Source files
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

# Main rule
all: $(TARGET)

# How to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# How to build the objects (.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Cleaning up compiled files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
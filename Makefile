CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET := load_balancer_sim

SOURCES := main.cpp Request.cpp RequestQueue.cpp WebServer.cpp LoadBalancer.cpp Switch.cpp
OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean run docs

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

docs:
	doxygen Doxyfile

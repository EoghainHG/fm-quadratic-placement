#compiler
CXX = g++
CXXFLAGS = -Wall -g

#target executable
TARGET = main

#delete executable when finished
TARGET_DEL = main.exe

#source files
SRCS = main.cpp cell.cpp bucketlist.cpp net.cpp NodeNet.cpp Parser.cpp renew.cpp SinglePassFM.cpp utils.cpp algorithm.cpp placement.cpp

#object files
OBJS = $(SRCS:.cpp=.o)

#rule to build and run the executable
all: $(TARGET) run

#rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

#rule to compile .cpp files into .o files
%.o: %.cpp	
	$(CXX) $(CXXFLAGS) -c $< -o $@

#rule to run the executable
run: $(TARGET)
	$(TARGET)

#clean rule to remove generated files
clean: 
	del $(TARGET_DEL) $(OBJS)
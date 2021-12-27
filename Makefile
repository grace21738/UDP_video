CC = gcc
CXX = g++
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv`
LINK_PTHREAD = -lpthread

RECEIVER = receiver.cpp
SENDER = sender.cpp
AGENT = agent.cpp
OPEN_CV = openCV.cpp
PTHREAD = pthread.c
SEN = sender
REC = receiver
AGE = agent
CV = openCV
PTH = pthread

all: sender receiver agent opencv
  
sender: $(SENDER)
	$(CXX) $(SENDER) $(INCLUDE_OPENCV) $(LINK_PTHREAD) -o $(SEN)  
receiver: $(RECEIVER)
	$(CXX) $(RECEIVER) $(INCLUDE_OPENCV) -Wall -o $(REC)
agent: $(AGENT)
	$(CXX) $(AGENT) $(INCLUDE_OPENCV) -o $(AGE)  
opencv: $(OPEN_CV)
	$(CXX) $(OPEN_CV) -o $(CV) $(INCLUDE_OPENCV) -o $(CV) 

.PHONY: clean

clean:
	rm $(SEN) $(REC) $(CV) $(AGE)
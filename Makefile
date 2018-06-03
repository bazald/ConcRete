CC=gcc
CXX=g++
RM=rm -f
# CPPFLAGS=-g $(shell root-config --cflags)
# LDFLAGS=-g $(shell root-config --ldflags)
# LDLIBS=$(shell root-config --libs)
CPPFLAGS=-std=c++17 -g -fPIC -fvisibility=hidden -fvisibility-inlines-hidden -Wall -Iinclude -IPEGTL/include
LDFLAGS=-g -L. -Wl,-rpath,. -Wl,-rpath-link,.
LDLIBS=-pthread

CONCURRENCY_SRCS=$(wildcard src/Zeni/Concurrency/*.cpp) src/Zeni/New_and_Delete.cpp src/Zeni/Utility.cpp
CONCURRENCY_OBJS=$(subst .cpp,.o,$(CONCURRENCY_SRCS))

RETE_SRCS=src/Zeni/New_and_Delete.cpp src/Zeni/Utility.cpp
RETE_SRCS+=src/Zeni/Rete/Custom_Data.cpp
RETE_SRCS+=src/Zeni/Rete/Network.cpp
RETE_SRCS+=src/Zeni/Rete/Node_Action.cpp
RETE_SRCS+=src/Zeni/Rete/Node.cpp
RETE_SRCS+=src/Zeni/Rete/Node_Filter.cpp
RETE_SRCS+=src/Zeni/Rete/Node_Passthrough.cpp
RETE_SRCS+=src/Zeni/Rete/Node_Passthrough_Gated.cpp
RETE_SRCS+=src/Zeni/Rete/Node_Unary.cpp
RETE_SRCS+=src/Zeni/Rete/Node_Unary_Gate.cpp
RETE_SRCS+=src/Zeni/Rete/Parser.cpp
RETE_SRCS+=$(wildcard src/Zeni/Rete/Raven*.cpp)
RETE_SRCS+=src/Zeni/Rete/Symbol.cpp
RETE_SRCS+=$(wildcard src/Zeni/Rete/Token*.cpp)
RETE_SRCS+=$(wildcard src/Zeni/Rete/Variable_*.cpp)
RETE_SRCS+=src/Zeni/Rete/WME.cpp
RETE_OBJS=$(subst .cpp,.o,$(RETE_SRCS))

TEST1_SRCS=$(wildcard Test1/*.cpp)
TEST1_OBJS=$(subst .cpp,.o,$(TEST1_SRCS))

BINARIES=libConcurrency.so libRete.so Test1.out

debug: CPPFLAGS+=-ggdb
debug: all

release: CPPFLAGS+=-O3
release: all

all: Test1.out

# tool: $(OBJS)
#     $(CXX) $(LDFLAGS) -o tool $(OBJS) $(LDLIBS) 

Concurrency: $(CONCURRENCY_OBJS)
	$(CXX) $(LDFLAGS) -shared -o libConcurrency.so $(CONCURRENCY_OBJS) $(LDLIBS) 

Rete: Concurrency $(RETE_OBJS)
	$(CXX) $(LDFLAGS) -shared -o libRete.so -lConcurrency $(RETE_OBJS) $(LDLIBS) 

Test1.out: Concurrency Rete $(TEST1_OBJS)
	$(CXX) $(LDFLAGS) -o Test1.out -lConcurrency -lRete $(TEST1_OBJS) $(LDLIBS) 

Test1/Test1.o: Concurrency Rete $(TEST1_SRCS)
	$(CXX) $(CPPFLAGS) -fno-PIC -o Test1/Test1.o -c Test1/Test1.cpp 

depend: .depend

.depend: $(CONCURRENCY_SRCS) $(RETE_SRCS) $(TEST1_SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(CONCURRENCY_OBJS) $(RETE_OBJS) $(TEST1_OBJS) $(BINARIES)

distclean: clean
	$(RM) *~ .depend

include .depend

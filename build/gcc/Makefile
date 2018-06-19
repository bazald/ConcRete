CC=gcc
CXX=g++
MKDIR=mkdir -p
RM=rm -f
RMDIR=rm -rf

# CPPFLAGS=-g $(shell root-config --cflags)
# LDFLAGS=-g $(shell root-config --ldflags)
# LDLIBS=$(shell root-config --libs)
CPPFLAGS=-std=c++17 -fvisibility=hidden -fvisibility-inlines-hidden -Wall -I../../include -I../../PEGTL/include
LDFLAGS=-L. -Wl,-rpath,. -Wl,-rpath-link,.
LDLIBS=-pthread

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SOURCES_SHARED=$(wildcard ../../src/Zeni/*.cpp)
SOURCES_CONCURRENCY=$(call rwildcard, ../../src/Zeni/Concurrency, *.cpp) $(SOURCES_SHARED)
SOURCES_RETE=$(call rwildcard, ../../src/Zeni/Rete, *.cpp) $(SOURCES_SHARED)
SOURCES_TEST1=$(call rwildcard, ../../Test1, *.cpp) $(SOURCES_SHARED)

OBJS_CONCURRENCY=$(addprefix Concurrency/, $(subst .cpp,.o,$(notdir $(SOURCES_CONCURRENCY))))
OBJS_RETE=$(addprefix Rete/, $(subst .cpp,.o,$(notdir $(SOURCES_RETE))))
OBJS_TEST1=$(addprefix Test1/, $(subst .cpp,.o,$(notdir $(SOURCES_TEST1))))

BINARIES=libConcurrency.so libRete.so Test1.out
DIRECTORIES=Concurrency Rete Test1
OBJS=$(OBJS_CONCURRENCY) $(OBJS_RETE) $(OBJS_TEST1)
SOURCES=$(call rwildcard, ../../src, *.cpp)

debug: CPPFLAGS+=-ggdb
debug: LDFLAGS+=-ggdb
debug: all

release: CPPFLAGS+=-O3 -DNDEBUG
release: all

.PHONY: all clean depend directories distclean

all: directories libConcurrency.so libRete.so Test1.out

directories: $(DIRECTORIES)

$(DIRECTORIES):
	$(MKDIR) $(DIRECTORIES)

libConcurrency.so: $(OBJS_CONCURRENCY)
	$(CXX) $(LDFLAGS) -shared -o $@ $(OBJS_CONCURRENCY) $(LDLIBS) 

Concurrency/%.o: $(SOURCES_CONCURRENCY)
	$(CXX) $(CPPFLAGS) -fPIC -o $@ -c $(filter %$(subst .o,.cpp,$(notdir $@)), $(SOURCES_CONCURRENCY))

libRete.so: libConcurrency.so $(OBJS_RETE)
	$(CXX) $(LDFLAGS) -shared -o $@ -lConcurrency $(OBJS_RETE) $(LDLIBS) 

Rete/%.o: $(SOURCES_RETE)
	$(CXX) $(CPPFLAGS) -fPIC -o $@ -c $(filter %$(subst .o,.cpp,$(notdir $@)), $(SOURCES_RETE))

Test1.out: libConcurrency.so libRete.so $(OBJS_TEST1)
	$(CXX) $(LDFLAGS) -o $@ -lConcurrency -lRete $(OBJS_TEST1) $(LDLIBS) 

Test1/%.o: $(SOURCES_TEST1)
	$(CXX) $(CPPFLAGS) -o $@ -c $(filter %$(subst .o,.cpp,$(notdir $@)), $(SOURCES_TEST1))

depend: .depend

.depend: $(SOURCES)
	$(CXX) $(CPPFLAGS) -MM $^ > ./.depend;

clean:
	$(RMDIR) Concurrency Rete Test1 $(BINARIES)

distclean: clean
	$(RM) .depend

include .depend

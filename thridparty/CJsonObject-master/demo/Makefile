CC = gcc
CXX = g++
CFLAGS = -std=c++11 -g -O2 -Wall -ggdb -m64 -D_GNU_SOURCE=1 -D_REENTRANT -D__GUNC__ 
cplusplus_version=$(shell g++ -dumpversion | awk '{if ($$NF > 5.0) print "c++14"; else print "c++11";}')
#CXXFLAG = -std=$(cplusplus_version) -g -O2 -Wall -ggdb -m64 -D_GNU_SOURCE=1 -D_REENTRANT -D__GUNC__ 
CXXFLAG = -std=c++11 -g -O2 -Wall -ggdb -m64 -D_GNU_SOURCE=1 -D_REENTRANT -D__GUNC__ 

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

VPATH = . ..
DIRS=$(VPATH)


INC := $(INC) 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ 

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))


TARGET = CJsonObjectTest

all: $(TARGET)

CJsonObjectTest:$(OBJS)
	$(CXX) -g -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

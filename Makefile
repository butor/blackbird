# Blackbird Bitcoin Arbitrage Makefile

override INC_DIR += -I ./src -I ./extern/sqlite3/include
override LIB_DIR += -L . 

CFLAGS   := -std=c99
CXXFLAGS := -Wall -pedantic -std=c++11 -Wno-missing-braces
LDFLAGS  := 
LDLIBS   := -lsqlite3 -lcrypto -ljansson -lcurl
CC       := gcc
OSNAME	 := $(shell uname)

EXEC = blackbird
SOURCES = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

SQLITE3 = libsqlite3.a
SQLITE3CLI = sqlite3
SQLITE3LIBS :=

ifeq ($(OSNAME),Darwin)
override INC_DIR += -I /usr/local/opt/openssl/include -I /usr/local/opt/jansson/include/
override LIB_DIR += -L /usr/local/opt/openssl/lib -L /usr/local/opt/jansson/lib/
endif

ifndef VERBOSE
  Q := @
else
  Q :=
endif

ifndef DEBUG
  CFLAGS   += -O2 -DNDEBUG
  CXXFLAGS += -O2 -DNDEBUG
else
  CFLAGS   += -O0 -g
  CXXFLAGS += -O0 -g
endif

ifneq ($(OS),Windows_NT)
  SQLITE3LIBS += -lpthread -ldl
endif

all: $(SQLITE3CLI) $(EXEC)

$(SQLITE3CLI): shell.o $(SQLITE3)
	@echo Linking $@:
	$(Q)$(CC) $(LDFLAGS) $(LIB_DIR) $< -o $@ -lsqlite3 $(SQLITE3LIBS)

$(SQLITE3): sqlite3.o
	@echo Archiving $@:
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(EXEC): $(SQLITE3) $(OBJECTS)
	@echo Linking $@:
	$(Q)$(CXX) $(LDFLAGS) $(LIB_DIR) $(OBJECTS) -o $@ $(LDLIBS) $(SQLITE3LIBS)

%.o: %.cpp
	@echo Compiling $@:
	$(Q)$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

%.o: extern/sqlite3/src/%.c
	@echo Compiling $@:
	$(Q)$(CC) $(CFLAGS) $(INC_DIR) -c $< -o $@

clean:
	$(Q)rm -rf core $(OBJECTS)

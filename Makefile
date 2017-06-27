# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.8

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.8.2/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.8.2/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Volumes/Workspace/crypto/arbitrage/blackbird

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Volumes/Workspace/crypto/arbitrage/blackbird

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/local/Cellar/cmake/3.8.2/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/usr/local/Cellar/cmake/3.8.2/bin/ccmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /Volumes/Workspace/crypto/arbitrage/blackbird/CMakeFiles /Volumes/Workspace/crypto/arbitrage/blackbird/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /Volumes/Workspace/crypto/arbitrage/blackbird/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named all_pch

# Build rule for target.
all_pch: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 all_pch
.PHONY : all_pch

# fast build rule for target.
all_pch/fast:
	$(MAKE) -f CMakeFiles/all_pch.dir/build.make CMakeFiles/all_pch.dir/build
.PHONY : all_pch/fast

#=============================================================================
# Target rules for targets named blackbird_pch

# Build rule for target.
blackbird_pch: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 blackbird_pch
.PHONY : blackbird_pch

# fast build rule for target.
blackbird_pch/fast:
	$(MAKE) -f CMakeFiles/blackbird_pch.dir/build.make CMakeFiles/blackbird_pch.dir/build
.PHONY : blackbird_pch/fast

#=============================================================================
# Target rules for targets named clean_cotire

# Build rule for target.
clean_cotire: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 clean_cotire
.PHONY : clean_cotire

# fast build rule for target.
clean_cotire/fast:
	$(MAKE) -f CMakeFiles/clean_cotire.dir/build.make CMakeFiles/clean_cotire.dir/build
.PHONY : clean_cotire/fast

#=============================================================================
# Target rules for targets named all_unity

# Build rule for target.
all_unity: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 all_unity
.PHONY : all_unity

# fast build rule for target.
all_unity/fast:
	$(MAKE) -f CMakeFiles/all_unity.dir/build.make CMakeFiles/all_unity.dir/build
.PHONY : all_unity/fast

#=============================================================================
# Target rules for targets named blackbird_unity

# Build rule for target.
blackbird_unity: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 blackbird_unity
.PHONY : blackbird_unity

# fast build rule for target.
blackbird_unity/fast:
	$(MAKE) -f CMakeFiles/blackbird_unity.dir/build.make CMakeFiles/blackbird_unity.dir/build
.PHONY : blackbird_unity/fast

#=============================================================================
# Target rules for targets named blackbird

# Build rule for target.
blackbird: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 blackbird
.PHONY : blackbird

# fast build rule for target.
blackbird/fast:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/build
.PHONY : blackbird/fast

cotire/blackbird_CXX_unity.o: cotire/blackbird_CXX_unity.cxx.o

.PHONY : cotire/blackbird_CXX_unity.o

# target to build an object file
cotire/blackbird_CXX_unity.cxx.o:
	$(MAKE) -f CMakeFiles/blackbird_unity.dir/build.make CMakeFiles/blackbird_unity.dir/cotire/blackbird_CXX_unity.cxx.o
.PHONY : cotire/blackbird_CXX_unity.cxx.o

cotire/blackbird_CXX_unity.i: cotire/blackbird_CXX_unity.cxx.i

.PHONY : cotire/blackbird_CXX_unity.i

# target to preprocess a source file
cotire/blackbird_CXX_unity.cxx.i:
	$(MAKE) -f CMakeFiles/blackbird_unity.dir/build.make CMakeFiles/blackbird_unity.dir/cotire/blackbird_CXX_unity.cxx.i
.PHONY : cotire/blackbird_CXX_unity.cxx.i

cotire/blackbird_CXX_unity.s: cotire/blackbird_CXX_unity.cxx.s

.PHONY : cotire/blackbird_CXX_unity.s

# target to generate assembly for a file
cotire/blackbird_CXX_unity.cxx.s:
	$(MAKE) -f CMakeFiles/blackbird_unity.dir/build.make CMakeFiles/blackbird_unity.dir/cotire/blackbird_CXX_unity.cxx.s
.PHONY : cotire/blackbird_CXX_unity.cxx.s

src/bitcoin.o: src/bitcoin.cpp.o

.PHONY : src/bitcoin.o

# target to build an object file
src/bitcoin.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/bitcoin.cpp.o
.PHONY : src/bitcoin.cpp.o

src/bitcoin.i: src/bitcoin.cpp.i

.PHONY : src/bitcoin.i

# target to preprocess a source file
src/bitcoin.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/bitcoin.cpp.i
.PHONY : src/bitcoin.cpp.i

src/bitcoin.s: src/bitcoin.cpp.s

.PHONY : src/bitcoin.s

# target to generate assembly for a file
src/bitcoin.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/bitcoin.cpp.s
.PHONY : src/bitcoin.cpp.s

src/check_entry_exit.o: src/check_entry_exit.cpp.o

.PHONY : src/check_entry_exit.o

# target to build an object file
src/check_entry_exit.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/check_entry_exit.cpp.o
.PHONY : src/check_entry_exit.cpp.o

src/check_entry_exit.i: src/check_entry_exit.cpp.i

.PHONY : src/check_entry_exit.i

# target to preprocess a source file
src/check_entry_exit.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/check_entry_exit.cpp.i
.PHONY : src/check_entry_exit.cpp.i

src/check_entry_exit.s: src/check_entry_exit.cpp.s

.PHONY : src/check_entry_exit.s

# target to generate assembly for a file
src/check_entry_exit.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/check_entry_exit.cpp.s
.PHONY : src/check_entry_exit.cpp.s

src/curl_fun.o: src/curl_fun.cpp.o

.PHONY : src/curl_fun.o

# target to build an object file
src/curl_fun.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/curl_fun.cpp.o
.PHONY : src/curl_fun.cpp.o

src/curl_fun.i: src/curl_fun.cpp.i

.PHONY : src/curl_fun.i

# target to preprocess a source file
src/curl_fun.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/curl_fun.cpp.i
.PHONY : src/curl_fun.cpp.i

src/curl_fun.s: src/curl_fun.cpp.s

.PHONY : src/curl_fun.s

# target to generate assembly for a file
src/curl_fun.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/curl_fun.cpp.s
.PHONY : src/curl_fun.cpp.s

src/db_fun.o: src/db_fun.cpp.o

.PHONY : src/db_fun.o

# target to build an object file
src/db_fun.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/db_fun.cpp.o
.PHONY : src/db_fun.cpp.o

src/db_fun.i: src/db_fun.cpp.i

.PHONY : src/db_fun.i

# target to preprocess a source file
src/db_fun.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/db_fun.cpp.i
.PHONY : src/db_fun.cpp.i

src/db_fun.s: src/db_fun.cpp.s

.PHONY : src/db_fun.s

# target to generate assembly for a file
src/db_fun.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/db_fun.cpp.s
.PHONY : src/db_fun.cpp.s

src/exchanges/bitfinex.o: src/exchanges/bitfinex.cpp.o

.PHONY : src/exchanges/bitfinex.o

# target to build an object file
src/exchanges/bitfinex.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/bitfinex.cpp.o
.PHONY : src/exchanges/bitfinex.cpp.o

src/exchanges/bitfinex.i: src/exchanges/bitfinex.cpp.i

.PHONY : src/exchanges/bitfinex.i

# target to preprocess a source file
src/exchanges/bitfinex.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/bitfinex.cpp.i
.PHONY : src/exchanges/bitfinex.cpp.i

src/exchanges/bitfinex.s: src/exchanges/bitfinex.cpp.s

.PHONY : src/exchanges/bitfinex.s

# target to generate assembly for a file
src/exchanges/bitfinex.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/bitfinex.cpp.s
.PHONY : src/exchanges/bitfinex.cpp.s

src/exchanges/bitstamp.o: src/exchanges/bitstamp.cpp.o

.PHONY : src/exchanges/bitstamp.o

# target to build an object file
src/exchanges/bitstamp.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/bitstamp.cpp.o
.PHONY : src/exchanges/bitstamp.cpp.o

src/exchanges/bitstamp.i: src/exchanges/bitstamp.cpp.i

.PHONY : src/exchanges/bitstamp.i

# target to preprocess a source file
src/exchanges/bitstamp.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/bitstamp.cpp.i
.PHONY : src/exchanges/bitstamp.cpp.i

src/exchanges/bitstamp.s: src/exchanges/bitstamp.cpp.s

.PHONY : src/exchanges/bitstamp.s

# target to generate assembly for a file
src/exchanges/bitstamp.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/bitstamp.cpp.s
.PHONY : src/exchanges/bitstamp.cpp.s

src/exchanges/btce.o: src/exchanges/btce.cpp.o

.PHONY : src/exchanges/btce.o

# target to build an object file
src/exchanges/btce.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/btce.cpp.o
.PHONY : src/exchanges/btce.cpp.o

src/exchanges/btce.i: src/exchanges/btce.cpp.i

.PHONY : src/exchanges/btce.i

# target to preprocess a source file
src/exchanges/btce.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/btce.cpp.i
.PHONY : src/exchanges/btce.cpp.i

src/exchanges/btce.s: src/exchanges/btce.cpp.s

.PHONY : src/exchanges/btce.s

# target to generate assembly for a file
src/exchanges/btce.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/btce.cpp.s
.PHONY : src/exchanges/btce.cpp.s

src/exchanges/gdax.o: src/exchanges/gdax.cpp.o

.PHONY : src/exchanges/gdax.o

# target to build an object file
src/exchanges/gdax.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/gdax.cpp.o
.PHONY : src/exchanges/gdax.cpp.o

src/exchanges/gdax.i: src/exchanges/gdax.cpp.i

.PHONY : src/exchanges/gdax.i

# target to preprocess a source file
src/exchanges/gdax.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/gdax.cpp.i
.PHONY : src/exchanges/gdax.cpp.i

src/exchanges/gdax.s: src/exchanges/gdax.cpp.s

.PHONY : src/exchanges/gdax.s

# target to generate assembly for a file
src/exchanges/gdax.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/gdax.cpp.s
.PHONY : src/exchanges/gdax.cpp.s

src/exchanges/gemini.o: src/exchanges/gemini.cpp.o

.PHONY : src/exchanges/gemini.o

# target to build an object file
src/exchanges/gemini.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/gemini.cpp.o
.PHONY : src/exchanges/gemini.cpp.o

src/exchanges/gemini.i: src/exchanges/gemini.cpp.i

.PHONY : src/exchanges/gemini.i

# target to preprocess a source file
src/exchanges/gemini.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/gemini.cpp.i
.PHONY : src/exchanges/gemini.cpp.i

src/exchanges/gemini.s: src/exchanges/gemini.cpp.s

.PHONY : src/exchanges/gemini.s

# target to generate assembly for a file
src/exchanges/gemini.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/gemini.cpp.s
.PHONY : src/exchanges/gemini.cpp.s

src/exchanges/itbit.o: src/exchanges/itbit.cpp.o

.PHONY : src/exchanges/itbit.o

# target to build an object file
src/exchanges/itbit.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/itbit.cpp.o
.PHONY : src/exchanges/itbit.cpp.o

src/exchanges/itbit.i: src/exchanges/itbit.cpp.i

.PHONY : src/exchanges/itbit.i

# target to preprocess a source file
src/exchanges/itbit.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/itbit.cpp.i
.PHONY : src/exchanges/itbit.cpp.i

src/exchanges/itbit.s: src/exchanges/itbit.cpp.s

.PHONY : src/exchanges/itbit.s

# target to generate assembly for a file
src/exchanges/itbit.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/itbit.cpp.s
.PHONY : src/exchanges/itbit.cpp.s

src/exchanges/kraken.o: src/exchanges/kraken.cpp.o

.PHONY : src/exchanges/kraken.o

# target to build an object file
src/exchanges/kraken.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/kraken.cpp.o
.PHONY : src/exchanges/kraken.cpp.o

src/exchanges/kraken.i: src/exchanges/kraken.cpp.i

.PHONY : src/exchanges/kraken.i

# target to preprocess a source file
src/exchanges/kraken.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/kraken.cpp.i
.PHONY : src/exchanges/kraken.cpp.i

src/exchanges/kraken.s: src/exchanges/kraken.cpp.s

.PHONY : src/exchanges/kraken.s

# target to generate assembly for a file
src/exchanges/kraken.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/kraken.cpp.s
.PHONY : src/exchanges/kraken.cpp.s

src/exchanges/okcoin.o: src/exchanges/okcoin.cpp.o

.PHONY : src/exchanges/okcoin.o

# target to build an object file
src/exchanges/okcoin.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/okcoin.cpp.o
.PHONY : src/exchanges/okcoin.cpp.o

src/exchanges/okcoin.i: src/exchanges/okcoin.cpp.i

.PHONY : src/exchanges/okcoin.i

# target to preprocess a source file
src/exchanges/okcoin.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/okcoin.cpp.i
.PHONY : src/exchanges/okcoin.cpp.i

src/exchanges/okcoin.s: src/exchanges/okcoin.cpp.s

.PHONY : src/exchanges/okcoin.s

# target to generate assembly for a file
src/exchanges/okcoin.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/okcoin.cpp.s
.PHONY : src/exchanges/okcoin.cpp.s

src/exchanges/poloniex.o: src/exchanges/poloniex.cpp.o

.PHONY : src/exchanges/poloniex.o

# target to build an object file
src/exchanges/poloniex.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/poloniex.cpp.o
.PHONY : src/exchanges/poloniex.cpp.o

src/exchanges/poloniex.i: src/exchanges/poloniex.cpp.i

.PHONY : src/exchanges/poloniex.i

# target to preprocess a source file
src/exchanges/poloniex.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/poloniex.cpp.i
.PHONY : src/exchanges/poloniex.cpp.i

src/exchanges/poloniex.s: src/exchanges/poloniex.cpp.s

.PHONY : src/exchanges/poloniex.s

# target to generate assembly for a file
src/exchanges/poloniex.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/exchanges/poloniex.cpp.s
.PHONY : src/exchanges/poloniex.cpp.s

src/main.o: src/main.cpp.o

.PHONY : src/main.o

# target to build an object file
src/main.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/main.cpp.o
.PHONY : src/main.cpp.o

src/main.i: src/main.cpp.i

.PHONY : src/main.i

# target to preprocess a source file
src/main.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/main.cpp.i
.PHONY : src/main.cpp.i

src/main.s: src/main.cpp.s

.PHONY : src/main.s

# target to generate assembly for a file
src/main.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/main.cpp.s
.PHONY : src/main.cpp.s

src/parameters.o: src/parameters.cpp.o

.PHONY : src/parameters.o

# target to build an object file
src/parameters.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/parameters.cpp.o
.PHONY : src/parameters.cpp.o

src/parameters.i: src/parameters.cpp.i

.PHONY : src/parameters.i

# target to preprocess a source file
src/parameters.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/parameters.cpp.i
.PHONY : src/parameters.cpp.i

src/parameters.s: src/parameters.cpp.s

.PHONY : src/parameters.s

# target to generate assembly for a file
src/parameters.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/parameters.cpp.s
.PHONY : src/parameters.cpp.s

src/result.o: src/result.cpp.o

.PHONY : src/result.o

# target to build an object file
src/result.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/result.cpp.o
.PHONY : src/result.cpp.o

src/result.i: src/result.cpp.i

.PHONY : src/result.i

# target to preprocess a source file
src/result.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/result.cpp.i
.PHONY : src/result.cpp.i

src/result.s: src/result.cpp.s

.PHONY : src/result.s

# target to generate assembly for a file
src/result.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/result.cpp.s
.PHONY : src/result.cpp.s

src/time_fun.o: src/time_fun.cpp.o

.PHONY : src/time_fun.o

# target to build an object file
src/time_fun.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/time_fun.cpp.o
.PHONY : src/time_fun.cpp.o

src/time_fun.i: src/time_fun.cpp.i

.PHONY : src/time_fun.i

# target to preprocess a source file
src/time_fun.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/time_fun.cpp.i
.PHONY : src/time_fun.cpp.i

src/time_fun.s: src/time_fun.cpp.s

.PHONY : src/time_fun.s

# target to generate assembly for a file
src/time_fun.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/time_fun.cpp.s
.PHONY : src/time_fun.cpp.s

src/utils/base64.o: src/utils/base64.cpp.o

.PHONY : src/utils/base64.o

# target to build an object file
src/utils/base64.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/base64.cpp.o
.PHONY : src/utils/base64.cpp.o

src/utils/base64.i: src/utils/base64.cpp.i

.PHONY : src/utils/base64.i

# target to preprocess a source file
src/utils/base64.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/base64.cpp.i
.PHONY : src/utils/base64.cpp.i

src/utils/base64.s: src/utils/base64.cpp.s

.PHONY : src/utils/base64.s

# target to generate assembly for a file
src/utils/base64.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/base64.cpp.s
.PHONY : src/utils/base64.cpp.s

src/utils/restapi.o: src/utils/restapi.cpp.o

.PHONY : src/utils/restapi.o

# target to build an object file
src/utils/restapi.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/restapi.cpp.o
.PHONY : src/utils/restapi.cpp.o

src/utils/restapi.i: src/utils/restapi.cpp.i

.PHONY : src/utils/restapi.i

# target to preprocess a source file
src/utils/restapi.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/restapi.cpp.i
.PHONY : src/utils/restapi.cpp.i

src/utils/restapi.s: src/utils/restapi.cpp.s

.PHONY : src/utils/restapi.s

# target to generate assembly for a file
src/utils/restapi.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/restapi.cpp.s
.PHONY : src/utils/restapi.cpp.s

src/utils/send_email.o: src/utils/send_email.cpp.o

.PHONY : src/utils/send_email.o

# target to build an object file
src/utils/send_email.cpp.o:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/send_email.cpp.o
.PHONY : src/utils/send_email.cpp.o

src/utils/send_email.i: src/utils/send_email.cpp.i

.PHONY : src/utils/send_email.i

# target to preprocess a source file
src/utils/send_email.cpp.i:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/send_email.cpp.i
.PHONY : src/utils/send_email.cpp.i

src/utils/send_email.s: src/utils/send_email.cpp.s

.PHONY : src/utils/send_email.s

# target to generate assembly for a file
src/utils/send_email.cpp.s:
	$(MAKE) -f CMakeFiles/blackbird.dir/build.make CMakeFiles/blackbird.dir/src/utils/send_email.cpp.s
.PHONY : src/utils/send_email.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... all_pch"
	@echo "... blackbird_pch"
	@echo "... clean_cotire"
	@echo "... all_unity"
	@echo "... blackbird_unity"
	@echo "... blackbird"
	@echo "... cotire/blackbird_CXX_unity.o"
	@echo "... cotire/blackbird_CXX_unity.i"
	@echo "... cotire/blackbird_CXX_unity.s"
	@echo "... src/bitcoin.o"
	@echo "... src/bitcoin.i"
	@echo "... src/bitcoin.s"
	@echo "... src/check_entry_exit.o"
	@echo "... src/check_entry_exit.i"
	@echo "... src/check_entry_exit.s"
	@echo "... src/curl_fun.o"
	@echo "... src/curl_fun.i"
	@echo "... src/curl_fun.s"
	@echo "... src/db_fun.o"
	@echo "... src/db_fun.i"
	@echo "... src/db_fun.s"
	@echo "... src/exchanges/bitfinex.o"
	@echo "... src/exchanges/bitfinex.i"
	@echo "... src/exchanges/bitfinex.s"
	@echo "... src/exchanges/bitstamp.o"
	@echo "... src/exchanges/bitstamp.i"
	@echo "... src/exchanges/bitstamp.s"
	@echo "... src/exchanges/btce.o"
	@echo "... src/exchanges/btce.i"
	@echo "... src/exchanges/btce.s"
	@echo "... src/exchanges/gdax.o"
	@echo "... src/exchanges/gdax.i"
	@echo "... src/exchanges/gdax.s"
	@echo "... src/exchanges/gemini.o"
	@echo "... src/exchanges/gemini.i"
	@echo "... src/exchanges/gemini.s"
	@echo "... src/exchanges/itbit.o"
	@echo "... src/exchanges/itbit.i"
	@echo "... src/exchanges/itbit.s"
	@echo "... src/exchanges/kraken.o"
	@echo "... src/exchanges/kraken.i"
	@echo "... src/exchanges/kraken.s"
	@echo "... src/exchanges/okcoin.o"
	@echo "... src/exchanges/okcoin.i"
	@echo "... src/exchanges/okcoin.s"
	@echo "... src/exchanges/poloniex.o"
	@echo "... src/exchanges/poloniex.i"
	@echo "... src/exchanges/poloniex.s"
	@echo "... src/main.o"
	@echo "... src/main.i"
	@echo "... src/main.s"
	@echo "... src/parameters.o"
	@echo "... src/parameters.i"
	@echo "... src/parameters.s"
	@echo "... src/result.o"
	@echo "... src/result.i"
	@echo "... src/result.s"
	@echo "... src/time_fun.o"
	@echo "... src/time_fun.i"
	@echo "... src/time_fun.s"
	@echo "... src/utils/base64.o"
	@echo "... src/utils/base64.i"
	@echo "... src/utils/base64.s"
	@echo "... src/utils/restapi.o"
	@echo "... src/utils/restapi.i"
	@echo "... src/utils/restapi.s"
	@echo "... src/utils/send_email.o"
	@echo "... src/utils/send_email.i"
	@echo "... src/utils/send_email.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system


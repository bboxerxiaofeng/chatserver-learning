# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/moduo/src/client

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/moduo/src/client/build

# Include any dependencies generated for this target.
include CMakeFiles/ChatClient.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ChatClient.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ChatClient.dir/flags.make

CMakeFiles/ChatClient.dir/main.o: CMakeFiles/ChatClient.dir/flags.make
CMakeFiles/ChatClient.dir/main.o: ../main.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/moduo/src/client/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/ChatClient.dir/main.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/ChatClient.dir/main.o -c /home/moduo/src/client/main.cpp

CMakeFiles/ChatClient.dir/main.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ChatClient.dir/main.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/moduo/src/client/main.cpp > CMakeFiles/ChatClient.dir/main.i

CMakeFiles/ChatClient.dir/main.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ChatClient.dir/main.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/moduo/src/client/main.cpp -o CMakeFiles/ChatClient.dir/main.s

CMakeFiles/ChatClient.dir/main.o.requires:
.PHONY : CMakeFiles/ChatClient.dir/main.o.requires

CMakeFiles/ChatClient.dir/main.o.provides: CMakeFiles/ChatClient.dir/main.o.requires
	$(MAKE) -f CMakeFiles/ChatClient.dir/build.make CMakeFiles/ChatClient.dir/main.o.provides.build
.PHONY : CMakeFiles/ChatClient.dir/main.o.provides

CMakeFiles/ChatClient.dir/main.o.provides.build: CMakeFiles/ChatClient.dir/main.o

# Object files for target ChatClient
ChatClient_OBJECTS = \
"CMakeFiles/ChatClient.dir/main.o"

# External object files for target ChatClient
ChatClient_EXTERNAL_OBJECTS =

ChatClient: CMakeFiles/ChatClient.dir/main.o
ChatClient: CMakeFiles/ChatClient.dir/build.make
ChatClient: CMakeFiles/ChatClient.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable ChatClient"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ChatClient.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ChatClient.dir/build: ChatClient
.PHONY : CMakeFiles/ChatClient.dir/build

CMakeFiles/ChatClient.dir/requires: CMakeFiles/ChatClient.dir/main.o.requires
.PHONY : CMakeFiles/ChatClient.dir/requires

CMakeFiles/ChatClient.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ChatClient.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ChatClient.dir/clean

CMakeFiles/ChatClient.dir/depend:
	cd /home/moduo/src/client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/moduo/src/client /home/moduo/src/client /home/moduo/src/client/build /home/moduo/src/client/build /home/moduo/src/client/build/CMakeFiles/ChatClient.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ChatClient.dir/depend


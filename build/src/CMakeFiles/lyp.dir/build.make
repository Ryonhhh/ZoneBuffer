# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/wht/zalpBuffer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/wht/zalpBuffer/build

# Include any dependencies generated for this target.
include src/CMakeFiles/lyp.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/lyp.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/lyp.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/lyp.dir/flags.make

src/CMakeFiles/lyp.dir/common.cpp.o: src/CMakeFiles/lyp.dir/flags.make
src/CMakeFiles/lyp.dir/common.cpp.o: ../src/common.cpp
src/CMakeFiles/lyp.dir/common.cpp.o: src/CMakeFiles/lyp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wht/zalpBuffer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/lyp.dir/common.cpp.o"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/lyp.dir/common.cpp.o -MF CMakeFiles/lyp.dir/common.cpp.o.d -o CMakeFiles/lyp.dir/common.cpp.o -c /home/wht/zalpBuffer/src/common.cpp

src/CMakeFiles/lyp.dir/common.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lyp.dir/common.cpp.i"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wht/zalpBuffer/src/common.cpp > CMakeFiles/lyp.dir/common.cpp.i

src/CMakeFiles/lyp.dir/common.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lyp.dir/common.cpp.s"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wht/zalpBuffer/src/common.cpp -o CMakeFiles/lyp.dir/common.cpp.s

src/CMakeFiles/lyp.dir/zalp.cpp.o: src/CMakeFiles/lyp.dir/flags.make
src/CMakeFiles/lyp.dir/zalp.cpp.o: ../src/zalp.cpp
src/CMakeFiles/lyp.dir/zalp.cpp.o: src/CMakeFiles/lyp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wht/zalpBuffer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/lyp.dir/zalp.cpp.o"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/lyp.dir/zalp.cpp.o -MF CMakeFiles/lyp.dir/zalp.cpp.o.d -o CMakeFiles/lyp.dir/zalp.cpp.o -c /home/wht/zalpBuffer/src/zalp.cpp

src/CMakeFiles/lyp.dir/zalp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lyp.dir/zalp.cpp.i"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wht/zalpBuffer/src/zalp.cpp > CMakeFiles/lyp.dir/zalp.cpp.i

src/CMakeFiles/lyp.dir/zalp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lyp.dir/zalp.cpp.s"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wht/zalpBuffer/src/zalp.cpp -o CMakeFiles/lyp.dir/zalp.cpp.s

src/CMakeFiles/lyp.dir/zBuffer.cpp.o: src/CMakeFiles/lyp.dir/flags.make
src/CMakeFiles/lyp.dir/zBuffer.cpp.o: ../src/zBuffer.cpp
src/CMakeFiles/lyp.dir/zBuffer.cpp.o: src/CMakeFiles/lyp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wht/zalpBuffer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/CMakeFiles/lyp.dir/zBuffer.cpp.o"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/lyp.dir/zBuffer.cpp.o -MF CMakeFiles/lyp.dir/zBuffer.cpp.o.d -o CMakeFiles/lyp.dir/zBuffer.cpp.o -c /home/wht/zalpBuffer/src/zBuffer.cpp

src/CMakeFiles/lyp.dir/zBuffer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lyp.dir/zBuffer.cpp.i"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wht/zalpBuffer/src/zBuffer.cpp > CMakeFiles/lyp.dir/zBuffer.cpp.i

src/CMakeFiles/lyp.dir/zBuffer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lyp.dir/zBuffer.cpp.s"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wht/zalpBuffer/src/zBuffer.cpp -o CMakeFiles/lyp.dir/zBuffer.cpp.s

src/CMakeFiles/lyp.dir/zController.cpp.o: src/CMakeFiles/lyp.dir/flags.make
src/CMakeFiles/lyp.dir/zController.cpp.o: ../src/zController.cpp
src/CMakeFiles/lyp.dir/zController.cpp.o: src/CMakeFiles/lyp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wht/zalpBuffer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/CMakeFiles/lyp.dir/zController.cpp.o"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/lyp.dir/zController.cpp.o -MF CMakeFiles/lyp.dir/zController.cpp.o.d -o CMakeFiles/lyp.dir/zController.cpp.o -c /home/wht/zalpBuffer/src/zController.cpp

src/CMakeFiles/lyp.dir/zController.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lyp.dir/zController.cpp.i"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wht/zalpBuffer/src/zController.cpp > CMakeFiles/lyp.dir/zController.cpp.i

src/CMakeFiles/lyp.dir/zController.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lyp.dir/zController.cpp.s"
	cd /home/wht/zalpBuffer/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wht/zalpBuffer/src/zController.cpp -o CMakeFiles/lyp.dir/zController.cpp.s

# Object files for target lyp
lyp_OBJECTS = \
"CMakeFiles/lyp.dir/common.cpp.o" \
"CMakeFiles/lyp.dir/zalp.cpp.o" \
"CMakeFiles/lyp.dir/zBuffer.cpp.o" \
"CMakeFiles/lyp.dir/zController.cpp.o"

# External object files for target lyp
lyp_EXTERNAL_OBJECTS =

src/liblyp.a: src/CMakeFiles/lyp.dir/common.cpp.o
src/liblyp.a: src/CMakeFiles/lyp.dir/zalp.cpp.o
src/liblyp.a: src/CMakeFiles/lyp.dir/zBuffer.cpp.o
src/liblyp.a: src/CMakeFiles/lyp.dir/zController.cpp.o
src/liblyp.a: src/CMakeFiles/lyp.dir/build.make
src/liblyp.a: src/CMakeFiles/lyp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wht/zalpBuffer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking CXX static library liblyp.a"
	cd /home/wht/zalpBuffer/build/src && $(CMAKE_COMMAND) -P CMakeFiles/lyp.dir/cmake_clean_target.cmake
	cd /home/wht/zalpBuffer/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lyp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/lyp.dir/build: src/liblyp.a
.PHONY : src/CMakeFiles/lyp.dir/build

src/CMakeFiles/lyp.dir/clean:
	cd /home/wht/zalpBuffer/build/src && $(CMAKE_COMMAND) -P CMakeFiles/lyp.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/lyp.dir/clean

src/CMakeFiles/lyp.dir/depend:
	cd /home/wht/zalpBuffer/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wht/zalpBuffer /home/wht/zalpBuffer/src /home/wht/zalpBuffer/build /home/wht/zalpBuffer/build/src /home/wht/zalpBuffer/build/src/CMakeFiles/lyp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/lyp.dir/depend


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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/local/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocmesh

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC

# Utility rule file for convertSERToQuoc.

# Include the progress variables for this target.
include tools/image/converter/CMakeFiles/convertSERToQuoc.dir/progress.make

tools/image/converter/CMakeFiles/convertSERToQuoc: tools/image/converter/convertSERToQuoc

convertSERToQuoc: tools/image/converter/CMakeFiles/convertSERToQuoc
convertSERToQuoc: tools/image/converter/CMakeFiles/convertSERToQuoc.dir/build.make
.PHONY : convertSERToQuoc

# Rule to build all files generated by this target.
tools/image/converter/CMakeFiles/convertSERToQuoc.dir/build: convertSERToQuoc
.PHONY : tools/image/converter/CMakeFiles/convertSERToQuoc.dir/build

tools/image/converter/CMakeFiles/convertSERToQuoc.dir/clean:
	cd /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/tools/image/converter && $(CMAKE_COMMAND) -P CMakeFiles/convertSERToQuoc.dir/cmake_clean.cmake
.PHONY : tools/image/converter/CMakeFiles/convertSERToQuoc.dir/clean

tools/image/converter/CMakeFiles/convertSERToQuoc.dir/depend:
	cd /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocmesh /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocmesh/tools/image/converter /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/tools/image/converter /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/tools/image/converter/CMakeFiles/convertSERToQuoc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools/image/converter/CMakeFiles/convertSERToQuoc.dir/depend


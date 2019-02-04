cmake_minimum_required(VERSION 3.10.2)
project(gb_emu_project)

set(CMAKE_CXX_STANDARD 17)

set(SRC_FILES
	src/audio.cpp
	src/binary_file_reader.cpp
	src/binary_file_writer.cpp
	src/cpu.cpp
	src/disassemble.cpp
	src/graphics.cpp
	src/joypad.cpp
	src/machine.cpp
	src/main.cpp
	src/memory.cpp
	src/rom.cpp
	src/timer.cpp
	src/mappers/mbc1.cpp
	src/mappers/mbc3.cpp
	src/mappers/mbc5.cpp
	src/mappers/plain_rom.cpp
)

if(MSVC)
	set(SDL2_DIR "third_party/SDL2")
	set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")
	else()
		set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x86")
	endif()
	set(SDL2_LIBRARIES "${SDL2_LIB_DIR}/SDL2;${SDL2_LIB_DIR}/SDL2main")
	set(SDL2_DLL "${SDL2_LIB_DIR}/SDL2.dll")
else()
	set(GCC_CXX_FLAGS "-O2 -flto -Wall -Wextra")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_FLAGS}")
	include(FindPkgConfig)
	pkg_search_module(SDL2 REQUIRED sdl2)
endif()

add_executable(gb_emu ${SRC_FILES})
target_include_directories(gb_emu PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(gb_emu ${SDL2_LIBRARIES})

if(MSVC)
	add_custom_command(TARGET gb_emu POST_BUILD COMMAND
		${CMAKE_COMMAND} -E copy_if_different ${SDL2_DLL} $<TARGET_FILE_DIR:gb_emu>)
endif()

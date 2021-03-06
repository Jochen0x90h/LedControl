cmake_minimum_required(VERSION 3.1)

# set project
project(Emulator)

# add custom module finders
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_VERBOSE_MAKEFILE ON)

# use C++14 standard by default
set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)
find_package(glfw3 REQUIRED)

# CLion: Effects are in External Libraries -> Header Search Paths -> Effects
set(EFFECTS
	../Effects/types.h
	../Effects/hsv2rgb.h
	../Effects/noise.h
	../Effects/Effect.h
	../Effects/Rainbow.h
	../Effects/Color.h
	../Effects/MovingLight.h
	../Effects/Spring.h
	../Effects/Summer.h
	../Effects/Autumn.h
	../Effects/Winter.h
	../Effects/Plasma.h
)
source_group(Effects FILES ${EFFECTS})

# CLion: Fonts are in External Libraries -> Header Search Paths -> Fonts
set(DISPLAY
	../Display/Bitmap.h
	../Display/Bitmap.cpp
	../Display/Font.h
	../Display/Font.cpp
	../Display/tahoma_8pt.h
	../Display/tahoma_8pt.cpp
)
source_group(Display FILES ${DISPLAY})

add_executable(emulator
	main.cpp
	glad/glad.h
	glad/glad.c
	Display.h
	Display.cpp
	Widget.cpp
	Widget.h
	LedStrip.cpp
	LedStrip.h
	Slider.cpp
	Slider.h
	util.h
	${EFFECTS}
	${DISPLAY}
)
include_directories(emulator
	PRIVATE
		.
		../Effects
		../Display
)
target_link_libraries(emulator
	PRIVATE
		glfw
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_DL_LIBS}
)


# execute generator before build
find_program(GENERATOR ledgenerator)

add_custom_target(generate
	COMMAND ${GENERATOR}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../Effects)
add_dependencies(emulator generate)

# hex generator
separate_arguments(GENERATE_HEX)
macro(generate_hex TARGET)
	if(NOT "${GENERATE_HEX}" STREQUAL "" AND NOT "${GENERATE_HEX}" STREQUAL "None")
		#message("*** Generate Hex for ${TARGET}: ${GENERATE_HEX}")
		add_custom_command(TARGET ${TARGET}
			POST_BUILD
			COMMAND ${GENERATE_HEX} ${TARGET} ${TARGET}.hex
		)
	endif()
endmacro()



if(${PLATFORM} STREQUAL "emu")
	set(BOARD "emu")
	set(NAME "${PROJECT_NAME}-${BOARD}")
	add_executable(${NAME}
		main.cpp
		EffectInfo.hpp
		E12.cpp
		E12.hpp
		Strip.hpp
		Timer.hpp
		effects/ColorFade.hpp
		effects/RgbFade.hpp
		effects/Strobe.hpp
		effects/CylonBounce.hpp
		effects/MeteorRain.hpp
		${BOARD}/Drivers.hpp
	)
	target_include_directories(${NAME}
		PRIVATE
			${BOARD}
	)
	target_link_libraries(${NAME}
		coco::coco
		coco-loop::coco-loop
		coco-buffer::coco-buffer
		coco-ledstrip::coco-ledstrip
		coco-noise::coco-noise
		coco-monodisplay::coco-monodisplay
		coco-flash::coco-flash
		coco-storage::coco-storage
	)
elseif(${PLATFORM} MATCHES "^nrf52")
	set(BOARD "LedControl")
	set(NAME "${PROJECT_NAME}-${BOARD}")

	add_executable(${NAME}
		main.cpp
		${BOARD}/debug.cpp
		${BOARD}/Drivers.hpp
		${BOARD}/SystemInit.cpp
	)
	target_include_directories(${NAME}
		PRIVATE
			${BOARD}
	)
	target_link_directories(${NAME}
		PRIVATE
			${BOARD} # find link.ld
	)
	target_link_libraries(${NAME}
		coco::coco
		coco-loop::coco-loop
		coco-buffer::coco-buffer
		coco-ledstrip::coco-ledstrip
		coco-noise::coco-noise
		coco-monodisplay::coco-monodisplay
		coco-flash::coco-flash
		coco-storage::coco-storage
	)

	# generate hex file for flashing the target
	generate_hex(${NAME})
endif()

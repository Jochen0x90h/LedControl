function(firmware FW TARGET)
	#message("*** Target: ${TARGET}")
	# check if target exists
	if(TARGET ${TARGET})
		string(REGEX REPLACE ".*\\:" "" BOARD ${TARGET})
		set(NAME "${FW}-${BOARD}")
		message("*** Firmware ${FW} on board ${BOARD}")

		add_executable(${NAME}
			EffectInfo.hpp
			EffectManager.cpp
			EffectManager.hpp
			E12.cpp
			E12.hpp
			Strip.hpp
			Timer.hpp
			effects/ColorCycle3.hpp
			effects/ColorFade.hpp
			effects/ColorFade3.hpp
			effects/CylonBounce.hpp
			effects/MeteorRain.hpp
			effects/StaticColor.hpp
			effects/Strobe.hpp
			effects/Spring.hpp
			effects/Summer.hpp
			effects/Autumn.hpp
			effects/Winter.hpp
			${FW}.cpp
			${BOARD}/${FW}.hpp
		)
		target_include_directories(${NAME}
			PRIVATE
				${BOARD}
		)
		target_link_libraries(${NAME}
			${TARGET}
			#coco::coco
			coco-loop::coco-loop
			coco-buffer::coco-buffer
			coco-led-strip::coco-led-strip
			coco-spi::coco-spi
			coco-mono-display::coco-mono-display
			coco-flash::coco-flash
			coco-storage::coco-storage
			coco-noise::coco-noise
		)

		# generate hex file for flashing the target
		if(${CMAKE_CROSSCOMPILING})
			#message("*** Generate Hex for ${NAME} using ${CMAKE_OBJCOPY}")
			add_custom_command(TARGET ${NAME}
				POST_BUILD
				COMMAND ${CMAKE_OBJCOPY} -O ihex ${NAME} ${NAME}.hex
			)
		endif()
	endif()
endfunction()

# led control board, see pcb subdirectory
if(${PLATFORM} STREQUAL "stm32g431xx")
	add_library(control
		control/debug.cpp
		control/SystemInit.cpp
	)
	target_include_directories(control
		PUBLIC
			control
	)
	target_link_libraries(control
		coco::coco
	)

	#set_target_properties(${NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}/link.ld)
	target_link_options(control
		PUBLIC
			"-T${CMAKE_CURRENT_SOURCE_DIR}/control/link.ld"
	)
endif()


firmware(LedControl coco-devboards::emu)
firmware(LedControl control)

firmware(Test coco-devboards::emu)
firmware(Test control)

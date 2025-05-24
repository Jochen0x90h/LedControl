function(firmware FW BOARD_LIB)
    #message("*** Target: ${TARGET}")
    # check if target exists
    if(TARGET ${BOARD_LIB})
        string(REGEX REPLACE ".*\\:" "" BOARD ${BOARD_LIB})
        set(NAME "${FW}-${BOARD}")
        message("*** Firmware ${FW} on board ${BOARD}")

        add_executable(${NAME}
            E12.cpp
            E12.hpp
            EffectInfo.hpp
            EffectManager.cpp
            EffectManager.hpp
            RemoteControl.cpp
            RemoteControl.hpp
            StripManager.cpp
            StripManager.hpp
            math.hpp
            Strip.hpp
            effects/ColorCycle3.hpp
            effects/ColorFade.hpp
            effects/ColorFade3.hpp
            effects/CylonBounce.hpp
            effects/MeteorRain.hpp
            effects/SnowSparkle.hpp
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
            ${BOARD_LIB}
            coco::coco
            coco-loop::coco-loop
            coco-device::coco-device
            coco-led-strip::coco-led-strip
            coco-spi::coco-spi
            coco-mono-display::coco-mono-display
            coco-ir::coco-ir
            coco-flash::coco-flash
            coco-storage::coco-storage
            coco-math::coco-math
        )

        # generate hex file for flashing the target
        if(${CMAKE_CROSSCOMPILING})
            set_target_properties(${NAME} PROPERTIES COMPILE_FLAGS "-save-temps -fverbose-asm")

            #message("*** Generate Hex for ${NAME} using ${CMAKE_OBJCOPY}")
            add_custom_command(TARGET ${NAME}
                POST_BUILD
                COMMAND ${CMAKE_OBJCOPY} -O ihex ${NAME} ${NAME}.hex
            )
        endif()
    endif()
endfunction()

# pcb "g431", see pcb subdirectory
if(${PLATFORM} STREQUAL "stm32g431xx")
    add_library(g431
        g431/debug.cpp
        g431/SystemInit.cpp
    )
    target_include_directories(g431
        PUBLIC
            g431
    )
    target_link_libraries(g431
        coco::coco
    )

    #set_target_properties(${NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}/link.ld)
    target_link_options(g431
        PUBLIC
            "-T${CMAKE_CURRENT_SOURCE_DIR}/g431/link.ld"
    )
endif()

# pcb "g431Triple" with 3 outputs, see pcb subdirectory
if(${PLATFORM} STREQUAL "stm32g431xx")
    add_library(g431Triple
        g431Triple/debug.cpp
        g431Triple/SystemInit.cpp
    )
    target_include_directories(g431Triple
        PUBLIC
            g431Triple
    )
    target_link_libraries(g431Triple
        coco::coco
    )

    #set_target_properties(${NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}/link.ld)
    target_link_options(g431Triple
        PUBLIC
            "-T${CMAKE_CURRENT_SOURCE_DIR}/g431Triple/link.ld"
    )
endif()


firmware(LedControl coco-devboards::emu)
firmware(LedControl g431)
firmware(LedControl g431Triple)

firmware(Test coco-devboards::emu)
firmware(Test g431)

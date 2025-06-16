add_executable(generator
    main.cpp
    colorTemperature.hpp
)
target_link_libraries(generator
    coco-math::coco-math
)

cd ~/Documents/Uni/Tasks/"Az iot"/Vital_Signs_Monitor/lcd_bringup

cat > CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

project(board_bringup)

enable_language(C ASM)

set(PROJECT_NAME board_bringup)

get_filename_component(
    CUBE_F7
    "${CMAKE_CURRENT_SOURCE_DIR}/../../STM32CubeF7"
    ABSOLUTE
)

add_executable(${PROJECT_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/components.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"

    "${CUBE_F7}/Drivers/STM32F7xx_HAL_Driver/Inc"
    "${CUBE_F7}/Drivers/STM32F7xx_HAL_Driver/Inc/Legacy"

    "${CUBE_F7}/Drivers/CMSIS/Device/ST/STM32F7xx/Include"
    "${CUBE_F7}/Drivers/CMSIS/Include"

    "${CUBE_F7}/Drivers/BSP/Components/rk043fn48h"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F746xx
)

target_compile_options(${PROJECT_NAME} PRIVATE
)

target_link_options(${PROJECT_NAME} PRIVATE
)

target_link_directories(${PROJECT_NAME} PRIVATE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
)
EOF
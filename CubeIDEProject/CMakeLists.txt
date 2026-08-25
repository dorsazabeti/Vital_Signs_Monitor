cmake_minimum_required(VERSION 3.20)

project(VitalSignsMonitor C ASM)

add_compile_definitions(
    STM32F746xx
    USE_HAL_DRIVER
    LV_CONF_INCLUDE_SIMPLE
)

set(CMAKE_C_STANDARD 11)

include_directories(
    .
    Core/Inc
    Drivers/CMSIS/Device/ST/STM32F7xx/Include
    Drivers/CMSIS/Include
    Drivers/STM32F7xx_HAL_Driver/Inc
    Drivers/STM32F7xx_HAL_Driver/Inc/Legacy
    lvgl
    lvgl/include
    FATFS/App
    FATFS/Target
    USB_HOST/App
    USB_HOST/Target
    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1
    Middlewares/Third_Party/FatFs/src
    Middlewares/ST/STM32_USB_Host_Library/Core/Inc
    Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc
    Middlewares/ST/STM32_USB_Host_Library/Core/Inc
    Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F746NGHX_FLASH.ld)
set(STARTUP_FILE Core/Startup/startup_stm32f746nghx.s)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    Core/Src/*.c
    Drivers/STM32F7xx_HAL_Driver/Src/*.c
    FATFS/App/*.c
    FATFS/Target/*.c
    USB_HOST/App/*.c
    USB_HOST/Target/*.c
    Middlewares/Third_Party/FreeRTOS/Source/*.c
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/*.c
    Middlewares/Third_Party/FatFs/src/*.c
    lvgl/src/*.c
    Middlewares/ST/STM32_USB_Host_Library/Core/Src/*.c
    Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Src/*.c
)

list(APPEND SOURCES ${STARTUP_FILE})

add_executable(${PROJECT_NAME}.elf ${SOURCES})

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -ffunction-sections
    -fdata-sections
    -Wall
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${LINKER_SCRIPT}
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
)

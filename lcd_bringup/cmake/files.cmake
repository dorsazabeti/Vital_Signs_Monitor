get_filename_component(
    CUBE_F7
    "${CMAKE_CURRENT_SOURCE_DIR}/../../STM32CubeF7"
    ABSOLUTE
)

set(HAL_DIR "${CUBE_F7}/Drivers/STM32F7xx_HAL_Driver")

target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f7xx_hal_msp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f7xx_it.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/system_stm32f7xx.c"

    "${CMAKE_CURRENT_SOURCE_DIR}/Src/syscall.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/startup_stm32f746xx.S"

    "${HAL_DIR}/Src/stm32f7xx_hal.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_cortex.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_gpio.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_rcc.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_rcc_ex.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_pwr.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_pwr_ex.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_ltdc.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_flash.c"
    "${HAL_DIR}/Src/stm32f7xx_hal_flash_ex.c"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/stm32f746xg_flash.ld"
    "${CMAKE_CURRENT_BINARY_DIR}"
    COPYONLY
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stm32f746xg_flash.ld"
)

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

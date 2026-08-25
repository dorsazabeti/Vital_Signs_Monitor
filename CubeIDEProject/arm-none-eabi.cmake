set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m7)

set(STM32_GNU_TOOLS_PATH
    "/Users/arad/Library/Application Support/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin"
    CACHE PATH "GNU Tools for STM32 binary directory")

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc
    HINTS "${STM32_GNU_TOOLS_PATH}" REQUIRED)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++
    HINTS "${STM32_GNU_TOOLS_PATH}" REQUIRED)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc
    HINTS "${STM32_GNU_TOOLS_PATH}" REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy
    HINTS "${STM32_GNU_TOOLS_PATH}" REQUIRED)
find_program(CMAKE_SIZE arm-none-eabi-size
    HINTS "${STM32_GNU_TOOLS_PATH}" REQUIRED)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")

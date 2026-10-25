cmake_minimum_required(VERSION 3.20)
option(ZEPHYR_FIRMWARE "Build real Zephyr firmware instead of skeleton libraries" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(ZEPHYR_FIRMWARE)
    set(BOARD apollo_stm32h743 CACHE STRING "Zephyr board")
    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
    project(apollo_fct_test LANGUAGES C)
    target_sources(app PRIVATE app/main.c)
    return()
endif()
project(apollo_fct_test LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PLATFORM "zephyr" CACHE STRING "Backend: zephyr or linux (compile-check skeletons)")
set_property(CACHE PLATFORM PROPERTY STRINGS zephyr linux)
if(PLATFORM STREQUAL "zephyr")
    set(OSAL_BACKEND zephyr)
elseif(PLATFORM STREQUAL "linux")
    set(OSAL_BACKEND posix)
else()
    message(FATAL_ERROR "Unsupported PLATFORM='${PLATFORM}'; use zephyr or linux")
endif()

# Static libraries only: no startup code, linker script or fake firmware image.
add_library(apollo_osal STATIC
    backends/osal/${OSAL_BACKEND}/osal_task.c
    backends/osal/${OSAL_BACKEND}/osal_mutex.c
    backends/osal/${OSAL_BACKEND}/osal_sem.c
    backends/osal/${OSAL_BACKEND}/osal_mq.c
    backends/osal/${OSAL_BACKEND}/osal_mem.c)
add_library(apollo_hal STATIC
    backends/hal/${PLATFORM}/hal_i2c.c
    backends/hal/${PLATFORM}/hal_spi.c
    backends/hal/${PLATFORM}/hal_uart.c
    backends/hal/${PLATFORM}/hal_can.c
    backends/hal/${PLATFORM}/hal_gpio.c
    backends/hal/${PLATFORM}/hal_adc.c
    backends/hal/${PLATFORM}/hal_pwm.c
    backends/hal/${PLATFORM}/hal_flash.c
    backends/hal/${PLATFORM}/hal_rng.c)
add_library(apollo_support STATIC
    backends/support/${PLATFORM}/console.c
    backends/support/${PLATFORM}/args.c
    backends/support/${PLATFORM}/config_source.c
    backends/support/${PLATFORM}/can_bus.c)

foreach(target IN ITEMS apollo_osal apollo_hal apollo_support)
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    if(PLATFORM STREQUAL "zephyr")
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/zephyr_stubs)
    endif()
endforeach()
target_link_libraries(apollo_support PUBLIC apollo_osal apollo_hal)
message(STATUS "${PLATFORM}: skeleton compile-check only; hardware operations are unavailable")

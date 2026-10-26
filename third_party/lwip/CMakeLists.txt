# Upstream sources are unchanged under vendor/. No upstream configure_file steps.
file(GLOB LWIP_CORE CONFIGURE_DEPENDS vendor/src/core/*.c vendor/src/core/ipv4/*.c)
file(GLOB LWIP_API CONFIGURE_DEPENDS vendor/src/api/*.c)
add_library(apollo_lwip STATIC ${LWIP_CORE} ${LWIP_API} vendor/src/netif/ethernet.c lwip_port.c)
target_include_directories(apollo_lwip PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/port
    ${CMAKE_CURRENT_SOURCE_DIR}/../..)
if(TARGET zephyr_interface)
    target_link_libraries(apollo_lwip PUBLIC zephyr_interface)
endif()

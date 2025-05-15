# Find OpenSSL library
# find_package(OpenSSL REQUIRED)

file (GLOB_RECURSE SRC_FILE ${CMAKE_CURRENT_LIST_DIR}/src/ptp4l/*.c)
string(REGEX REPLACE ".*/\(.*\)" "\\1" MODULE ${CMAKE_CURRENT_LIST_DIR})

set(MODULE "ptp4l")

find_file(KTIME_INCLUDE_FIND
  NAMES include/ktime.h
  PATHS ${KTIME_INSTALL}
  NO_DEFAULT_PATH
)

add_executable(${MODULE}
    ${SRC_FILE} ${FILTERS}
    ${SERVO} ${SECURITY} ${TRANSP})

target_link_libraries(${MODULE} PRIVATE m rt pthread)

if(KTIME_INCLUDE_FIND)
    set(CMAKE_C_FLAGS "-DUSE_KTIME ${CMAKE_C_FLAGS} -I${KTIME_INSTALL}/include")

    message("-- Find ktime in ${KTIME_INSTALL}")
    target_link_libraries(${MODULE} PRIVATE ${KTIME_INSTALL}/lib/libktime.so)
    install(FILES 
        ${KTIME_INSTALL}/lib/libktime.so
        ${KTIME_INSTALL}/lib/libktime.so.1.0
        ${KTIME_INSTALL}/lib/libktime.so.1.0.0
        DESTINATION lib
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
endif()

install(TARGETS ${MODULE})
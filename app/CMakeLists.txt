cmake_minimum_required (VERSION 3.6)

project (ptp VERSION 1.0.0 LANGUAGES C)

include(CheckIncludeFiles)

set(CMAKE_VERBOSE_MAKEFILE off)

option(ENABLE_SANITIZE_ADDR "Enable Address Sanitizer" OFF)

file (GLOB_RECURSE SRC_FILES src/*.c)

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

set(CMAKE_C_FLAGS "-fstack-protector -fstack-protector-all ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wunused -Wno-unused-function ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-fno-common ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-DFORTIFY_SOURCE=2 -DHAVE_ONESTEP_SYNC ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wno-address-of-packed-member ${CMAKE_C_FLAGS}")

if(ENABLE_SANITIZE_ADDR)
  set(CMAKE_C_FLAGS "-fsanitize=address ${CMAKE_C_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address ${CMAKE_EXE_LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address ${CMAKE_SHARED_LINKER_FLAGS}")
  add_definitions(
    -DASAN_DEFAULT_OPTIONS="verbosity=0:unmap_shadow_on_exit=1:disable_coredump=0:abort_on_error=1:detect_leaks=1")
endif(ENABLE_SANITIZE_ADDR)

file (GLOB_RECURSE FILTERS src/filters/*.c)
file (GLOB_RECURSE SERVO src/servo/*.c)
file (GLOB_RECURSE SECURITY src/sec/*.c)
file (GLOB_RECURSE TRANSP src/transp/*.c)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

include(ptp4l.cmake)

install(FILES 
    ${CMAKE_CURRENT_LIST_DIR}/start-ptp4l.sh
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
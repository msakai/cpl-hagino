cmake_minimum_required(VERSION 3.15)
project(CPL VERSION 4.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Add compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required libraries
find_package(Curses REQUIRED)

# Platform detection for PTY support
include(CheckIncludeFile)
check_include_file("termios.h" HAVE_TERMIOS_H)
check_include_file("pty.h" HAVE_PTY_H)
check_include_file("util.h" HAVE_UTIL_H)

if(NOT HAVE_TERMIOS_H)
    message(FATAL_ERROR "termios.h is required but not found")
endif()

# Configure platform-specific settings
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/config.h"
)

# Window Manager executable (C)
add_executable(wm
    src/wm.c
    src/winlib.c
    src/display.c
    src/term.c
)

target_include_directories(wm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(wm ${CURSES_LIBRARIES})

# Platform-specific libraries for PTY support
if(APPLE OR UNIX)
    target_link_libraries(wm util)
endif()

# CPL interpreter (SBCL)
find_program(SBCL sbcl)
if(SBCL)
    message(STATUS "Found SBCL: ${SBCL}")

    # Custom command to build CPL executable
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpl
        COMMAND ${SBCL}
                --noinform
                --no-sysinit
                --no-userinit
                --disable-debugger
                --load ${CMAKE_CURRENT_SOURCE_DIR}/src/build-executable.lisp
        DEPENDS
            src/cpl.asd
            src/cpl-package.lisp
            src/franz-compat.lisp
            src/wcathelp.lisp
            src/tv.lisp
            src/wmlib.lisp
            src/wdia.lisp
            src/wtrace.lisp
            src/wcat.lisp
            src/build-executable.lisp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building CPL interpreter with SBCL"
    )

    add_custom_target(cpl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cpl)

    # Installation for CPL
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cpl
            DESTINATION bin)
else()
    message(WARNING "SBCL not found - CPL interpreter will not be built")
endif()

# Installation
install(TARGETS wm DESTINATION bin)

install(FILES
    system/Nat
    system/List
    system/InfList
    system/CoNat
    system/Sort
    DESTINATION share/cpl/system
)

install(FILES
    CLAUDE.md
    DESTINATION share/doc/cpl
)

# Testing
enable_testing()

if(SBCL)
    add_test(
        NAME cpl_loads
        COMMAND ${SBCL} --noinform --no-sysinit --no-userinit --disable-debugger
                --eval "(require :asdf)"
                --eval "(push #p\"${CMAKE_CURRENT_SOURCE_DIR}/src/\" asdf:*central-registry*)"
                --eval "(asdf:load-system :cpl)"
                --eval "(sb-ext:exit :code 0)"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "CPL Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  SBCL: ${SBCL}")
message(STATUS "  Curses: ${CURSES_LIBRARIES}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")

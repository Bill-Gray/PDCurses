cmake_minimum_required(VERSION 3.11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to MinSizeRel.")
endif()

set(PDCURSES_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PDCURSES_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include (get_version)
project(pdcurses VERSION "${CURSES_VERSION}" LANGUAGES C)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX d)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")  # enable parallel builds
endif()

message(STATUS "Generator .............. ${CMAKE_GENERATOR}")
message(STATUS "Build Type ............. ${CMAKE_BUILD_TYPE}")

include(build_options)
include(build_dependencies)

message(STATUS "PDC Version ............ ${PROJECT_VERSION}")

include(gen_config_header)

file(GLOB pdcurses_src_files pdcurses/*.c)

if(CYGWIN)
    message(STATUS "Windows Kit UM lib path = ${WINDOWS_KIT_LIBRARY_DIR}")
    link_directories(${WINDOWS_KIT_LIBRARY_DIR})
endif()

if(PDC_DOS_BUILD) # currently requires a unique toolchain file

    add_subdirectory(dos)

elseif(PDC_DOSVGA_BUILD) # currently requires a unique toolchain file

    add_subdirectory(dosvga)

elseif(PDC_DOSVT_BUILD) # currently requires a unique toolchain file

    add_subdirectory(vt)

elseif(PDC_OS2_BUILD) # currently requires a unique toolchain file

    add_subdirectory(os2)
    
else()

    if (PDC_NCURSES_BUILD)
        add_subdirectory(ncurses)
    endif()
    
    if(PDC_SDL2_BUILD)
    
        add_subdirectory(sdl2)
        
    endif()

    if(UNIX OR WATCOM)
        add_subdirectory(vt)
    endif()

    if(WIN32)
        if (PDC_WINCON_BUILD)
            add_subdirectory(wincon)
        endif()
        if (PDC_WINGUI_BUILD)
            add_subdirectory(wingui)
        endif()
    endif()
   
endif()

add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/make_uninstall.cmake")

set(CPACK_COMPONENTS_ALL applications)

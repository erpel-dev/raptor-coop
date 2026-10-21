cmake_minimum_required(VERSION 3.16)
project(raptor)

set(EXECUTABLE_OUTPUT_PATH "bin")
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_C_STANDARD 11)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(BUILD_WITH_SETUP "Build project with setup" ON)

if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        SDL
        URL            https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.30.10.zip
        SOURCE_DIR     ${CMAKE_BINARY_DIR}/include/SDL2
        )
    set(BUILD_SHARED_LIBS ON)
    set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
    set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
    set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL)
    set(SDL_ACTIVITY_FOLDER app/src/main/java/org/libsdl/app/)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/${SDL_ACTIVITY_FOLDER})
    file(COPY ${CMAKE_BINARY_DIR}/include/SDL2/android-project/${SDL_ACTIVITY_FOLDER} DESTINATION ${CMAKE_SOURCE_DIR}/android/${SDL_ACTIVITY_FOLDER})
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/app/src/main/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/include/TinySoundFont/LICENSE DESTINATION ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses)
    file(RENAME ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses/LICENSE ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses/LICENSE.TSF)
    file(COPY ${CMAKE_SOURCE_DIR}/pkg/android/license/LICENSE DESTINATION ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses)
elseif(WIN32)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    if(MSVC)
        add_compile_options(/wd4996)
        set(SDL2_DIR "include/SDL2-devel-2.28.2-VC/SDL2-2.28.2/cmake")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
        set(SDL2_DIR "include/SDL2-devel-2.28.2-mingw/SDL2-2.28.2/cmake")
    endif()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
else()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

include_directories(./include/textscreen/include/)
include_directories(./include/TinySoundFont/)
include_directories(./include/miniz/)

add_subdirectory(./include/textscreen/include)

set(SOURCE_FILES
    src/tile.cpp                             src/tile.h
    src/gssapi.cpp                           src/gssapi.h
    src/objects.cpp                          src/objects.h
    src/tile_a.cpp
    src/windows.cpp                          src/windows.h
    src/flame.cpp                            src/flame.h
    src/i_lastscr.cpp                        src/i_lastscr.h
    src/joyapi.cpp                           src/joyapi.h
    src/kbdapi.cpp                           src/kbdapi.h
    src/glbapi.cpp                           src/glbapi.h
    src/i_video.cpp                          src/i_video.h
    src/musapi.cpp                           src/musapi.h
    src/movie_a.cpp
    src/ptrapi_a.cpp
    src/swdapi.cpp                           src/swdapi.h
    src/imsapi.cpp                           src/imsapi.h
    src/store.cpp                            src/store.h
    src/i_oplmusic.cpp                       src/i_oplmusic.h
    src/bonus.cpp                            src/bonus.h
    src/anims.cpp                            src/anims.h
    src/shadow.cpp                           src/shadow.h
    src/movie.cpp                            src/movie.h
    src/m_misc.cpp                           src/m_misc.h
    src/shots.cpp                            src/shots.h
    src/opl3.cpp                             src/opl3.h
    src/eshot.cpp                            src/eshot.h
    src/gfxapi_a.cpp
    src/dspapi.cpp                           src/dspapi.h
    src/vmemapi.cpp                          src/vmemapi.h
    src/rap.cpp                              src/rap.h
    src/demo.cpp                             src/demo.h
    src/prefapi.cpp                          src/prefapi.h
    src/gfxapi.cpp                           src/gfxapi.h
    src/intro.cpp                            src/intro.h
    src/help.cpp                             src/help.h
    src/input.cpp                            src/input.h
    src/fx.cpp                               src/fx.h
    src/ptrapi.cpp                           src/ptrapi.h
    src/loadsave.cpp                         src/loadsave.h
    src/enemy.cpp                            src/enemy.h
    src/mpuwinmm.cpp
    src/mputsf.cpp
    src/mpualsa.cpp
    src/mpucorea.cpp
    src/mpucorem.cpp
    src/assets_fetch.cpp
    src/player.cpp
    include/miniz/miniz.c
                                             src/cards.h
                                             src/common.h
                                             src/entypes.h
                                             src/fileids.h
                                             src/rapver.h
                                             src/winids.h
    )

if(BUILD_WITH_SETUP)
    list(APPEND SOURCE_FILES 
         src/setup/se_input.cpp                   src/setup/se_input.h
         src/setup/se_main.cpp                    src/setup/se_main.h
         src/setup/se_music.cpp                   src/setup/se_music.h
         src/setup/se_prefapi.cpp                 src/setup/se_prefapi.h
         src/setup/se_sound.cpp                   src/setup/se_sound.h
         src/setup/se_txt_joyinput.cpp            src/setup/se_txt_joyinput.h
         src/setup/se_txt_keyinput.cpp            src/setup/se_txt_keyinput.h
         src/setup/se_txt_mouseinput.cpp          src/setup/se_txt_mouseinput.h
         )
    add_compile_definitions(BUILD_WITH_SETUP=1)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    add_library( ${PROJECT_NAME} SHARED ${SOURCE_FILES})   
    target_link_libraries(raptor
        textscreen
        SDL2::SDL2
        )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(ALSA REQUIRED)
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_link_libraries(raptor
        textscreen
        asound
        SDL2::SDL2main
        SDL2::SDL2
        )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(AUDIO_TOOLBOX AudioToolbox)
    find_library(AUDIO_UNIT AudioUnit)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(COREMIDI_LIB CoreMIDI)
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_link_libraries(raptor ${AUDIO_TOOLBOX} ${AUDIO_UNIT} ${FOUNDATION_LIBRARY} ${COREMIDI_LIB}
        textscreen
        SDL2::SDL2main
        SDL2::SDL2
        )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_sources(raptor PRIVATE rsrc/resource.rc)
    target_link_libraries(raptor
        winmm.lib
        urlmon.lib
        textscreen
        SDL2::SDL2main
        SDL2::SDL2
        )
    if(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo" OR ${CMAKE_BUILD_TYPE} MATCHES "Release")
        if(MSVC)
            target_link_options(raptor PRIVATE /subsystem:windows /ENTRY:mainCRTStartup)
        else()
            target_link_options(raptor PRIVATE -mwindows)
        endif()
    elseif(NOT MSVC)
        target_link_options(raptor PRIVATE -mconsole)
    endif()
else()
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_link_libraries(raptor
        textscreen
        SDL2::SDL2main
        SDL2::SDL2
        )
endif()
if(NOT ANDROID)
    include(GNUInstallDirs)
    install(TARGETS raptor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES
        "${CMAKE_SOURCE_DIR}/SETUP(ADLIB).INI"
        "${CMAKE_SOURCE_DIR}/SETUP(MIDI).INI"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/raptor
    )
    if(UNIX AND NOT APPLE)
        install(FILES "${CMAKE_SOURCE_DIR}/rsrc/raptor.png"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
            RENAME io.github.erpel_dev.Raptor.png)
        install(FILES "${CMAKE_SOURCE_DIR}/pkg/flatpak/io.github.erpel_dev.Raptor.desktop"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        install(FILES "${CMAKE_SOURCE_DIR}/pkg/flatpak/io.github.erpel_dev.Raptor.metainfo.xml"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    endif()
endif()

# These tests exercise game state without loading GLB assets or opening a window.
option(RAPTOR_BUILD_TESTS "Build co-op regression tests (Linux)" OFF)
if(RAPTOR_BUILD_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    enable_testing()
    add_executable(coop_tests tests/coop_tests.cpp ${SOURCE_FILES})
    target_compile_definitions(coop_tests PRIVATE RAPTOR_TESTING=1)
    target_include_directories(coop_tests PRIVATE src)
    target_link_libraries(coop_tests textscreen asound SDL2::SDL2)
    add_test(NAME coop_regressions COMMAND coop_tests)
    set_tests_properties(coop_regressions PROPERTIES TIMEOUT 30)
endif()

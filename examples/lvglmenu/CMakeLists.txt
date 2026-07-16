
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #######

# ------------------------------------------------------------------------------------------------ #
#  LVGL Menu - a real encoder-navigated menu built on LVGL, not a demo widget                      #
# ------------------------------------------------------------------------------------------------ #

cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(lvgl-menu)

set(LV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h CACHE STRING "" FORCE)

# Pinned to the last LVGL v8 release - v9 changed the display/driver API this
# is written against (lv_disp_draw_buf_t, lv_disp_drv_t, etc no longer exist
# the same way in v9). Plain source tarball instead of a git clone - we don't
# need LVGL's git history for a vendored dependency, and it avoids git's
# ref-negotiation/pack overhead entirely.
FetchContent_Declare(lvgl
  URL https://github.com/lvgl/lvgl/archive/refs/tags/v8.3.11.tar.gz)
FetchContent_MakeAvailable(lvgl)

SET(
  Examples_LvglMenu_FILES
    main.cpp
    LvglMenu.h
    LvglMenu.cpp
    MenuUi.h
    MenuUi.cpp
)

add_executable(${PROJECT_NAME} ${Examples_LvglMenu_FILES})

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
    OUTPUT_NAME       ${PROJECT_NAME}
    OUTPUT_NAME_DEBUG ${PROJECT_NAME}${DEBUG_SUFFIX}
)

target_link_libraries(${PROJECT_NAME} PRIVATE cabl-static lvgl::lvgl)
target_include_directories(${PROJECT_NAME} PRIVATE ${lvgl_SOURCE_DIR} .)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreFoundation" "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreAudio" "-framework CoreMidi" objc)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# ------------------------------------------------------------------------------------------------ #
#  lvgl-menu-sim - terminal (Braille art) simulator, no MK1/USB hardware needed                    #
#  Shares MenuUi with lvgl-menu above; deliberately does NOT link cabl-static, so this stays a      #
#  lighter, hardware-independent binary for iterating on menu logic.                                #
# ------------------------------------------------------------------------------------------------ #

set(SIM_PROJECT_NAME lvgl-menu-sim)

SET(
  Examples_LvglMenuSim_FILES
    main_sim.cpp
    TerminalMenu.h
    TerminalMenu.cpp
    MenuUi.h
    MenuUi.cpp
)

add_executable(${SIM_PROJECT_NAME} ${Examples_LvglMenuSim_FILES})

set_target_properties(
  ${SIM_PROJECT_NAME}
  PROPERTIES
    OUTPUT_NAME       ${SIM_PROJECT_NAME}
    OUTPUT_NAME_DEBUG ${SIM_PROJECT_NAME}${DEBUG_SUFFIX}
)

target_link_libraries(${SIM_PROJECT_NAME} PRIVATE lvgl::lvgl)
target_include_directories(${SIM_PROJECT_NAME} PRIVATE ${lvgl_SOURCE_DIR} .)

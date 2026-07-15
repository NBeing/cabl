
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #######

# ------------------------------------------------------------------------------------------------ #
#  Button Debugger                                                                                 #
# ------------------------------------------------------------------------------------------------ #

project(button-debug)

SET(
  Examples_ButtonDebug_FILES
    main.cpp
    ButtonDebug.h
    ButtonDebug.cpp
    Mk1Led.h
)

add_executable(
  ${PROJECT_NAME}
  ${Examples_ButtonDebug_FILES}
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
    OUTPUT_NAME           ${PROJECT_NAME}
    OUTPUT_NAME_DEBUG     ${PROJECT_NAME}${DEBUG_SUFFIX}
)

target_link_libraries(${PROJECT_NAME} PRIVATE cabl-static)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreFoundation" "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreAudio" "-framework CoreMidi" objc)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

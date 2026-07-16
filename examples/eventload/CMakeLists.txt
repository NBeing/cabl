
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #######

# ------------------------------------------------------------------------------------------------ #
#  Event Load Harness - stress-tests RTSafeEventDistributor, no hardware required                  #
# ------------------------------------------------------------------------------------------------ #

project(event-load)

SET(
  Examples_EventLoad_FILES
    main.cpp
    EventLoadHarness.h
    EventLoadHarness.cpp
)

add_executable(
  ${PROJECT_NAME}
  ${Examples_EventLoad_FILES}
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

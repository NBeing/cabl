
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #######

# ------------------------------------------------------------------------------------------------ #
#  PlockSeq - Digitakt-style 16-step sequencer with per-step, per-synth-CC parameter locks          #
# ------------------------------------------------------------------------------------------------ #

project(plockseq)

SET(
  Examples_PlockSeq_FILES
    main.cpp
    PlockSeq.h
    PlockSeq.cpp
)

add_executable(
  ${PROJECT_NAME}
  ${Examples_PlockSeq_FILES}
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
    OUTPUT_NAME           ${PROJECT_NAME}
    OUTPUT_NAME_DEBUG     ${PROJECT_NAME}${DEBUG_SUFFIX}
)

target_link_libraries(${PROJECT_NAME} PRIVATE cabl-static seqlib)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreFoundation" "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreAudio" "-framework CoreMidi" objc)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

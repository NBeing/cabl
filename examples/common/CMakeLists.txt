
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #######

# ------------------------------------------------------------------------------------------------ #
#  seqlib - reusable, portable step-sequencer/p-lock building blocks shared across examples         #
#  (StepLane/Pattern/SynthProfile/IStepClock in core/, desktop-only backends in desktop/)            #
# ------------------------------------------------------------------------------------------------ #

project(seqlib)

# Header-only: just an include path, no binary to build/link.
add_library(seqlib INTERFACE)
target_include_directories(seqlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

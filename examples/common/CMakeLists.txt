
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

# StepLane.h uses std::atomic<T> for T's bigger than a machine word (e.g.
# NoteEvent, 3 bytes) - GCC/Clang implement those via libatomic's generic
# __atomic_load/__atomic_store rather than inline instructions, which needs
# an explicit link. Universally available (glibc, musl, and cross
# toolchains like ESP-IDF's all ship it), so this doesn't compromise the
# portability goal - it's the same class of dependency as linking pthread
# for std::thread.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_libraries(seqlib INTERFACE atomic)
endif()

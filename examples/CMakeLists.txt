
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #######

# ------------------------------------------------------------------------------------------------ #
#  Example projects                                                                                #
# ------------------------------------------------------------------------------------------------ #

# Euklid - Euclidean sequencer ------------------------------------------------------------------- #
add_subdirectory(device-test DeviceTest)
add_subdirectory(euklid Euklid)
add_subdirectory(knobdisplay KnobDisplay)
add_subdirectory(buttondebug ButtonDebug)
add_subdirectory(screentest ScreenTest)
# Note: colordisplay requires lodepng and is for devices with color displays (like Push2)
# Uncomment the next line if you want to build it:
# add_subdirectory(colordisplay ColorDisplay)

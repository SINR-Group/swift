###############################################################################
# Copyright 2006 -- Thomson R&D France 
#                   Technology - Corporate Research
###############################################################################
# File          : README.TXT
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 1.0.0
################################################################################


1) The current directory should contain the "short-term" simulations set to be validated.

2) The following sequences are needed: BUS_352x288_30.yuv, MOBILE_352x288_30.yuv, FOOTBALL_352x288_30.yuv and, CREW_704x576_60.yuv 

3) Note that you will have (to compile and) to use the PacketLossSimulatorStatic tool in order to run the ERRORCON1, ERRORCON2, ERRORCON3 and, ERRORCON4 valaidation tests.

4) In order to run the "Interlaced" validation scripts CFG1_P2I and, CFG4_I2I, MGSCtrl3, and MGSCtrl4 the user will have to download the SD sequence called src5_ref__625.yuv available at ftp://vqeg.its.bldrdoc.gov/SDTV/VQEG_PhaseI/TestSequences/ALL_625/. This sequence is in 4:2:2 format. Conversion to 4:2:0 format shall be made using the conversion tool 422_to_420fullres using the p option as following: 
--------------------------------------------------------------
">422_to_420perl.exe p src5_ref__625.yuv CANOA_720x576_25i.yuv"
--------------------------------------------------------------
The resulting CANOA_720x576_25i.yuv will then be used for the Interlaced validation scripts.


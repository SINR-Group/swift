###############################################################################
# Copyright 2007 -- Thomson R&D France 
#                   Technology - Corporate Research
###############################################################################
# File          : README.TXT
# Author        : jerome.vieron@thomson.net
# Creation date : 15 January 2007
# Version       : 1.0.0
################################################################################


1) The current directory  contains the source code of the SD 422 to 420 conversion tool proposed in [VCEG-059]. 
This tool has to be used for certain validation tests related to Interlaced material. 

2) In order to run the "Interlaced" validation scripts CFG1_P2I, CFG4_I2I,  and LCFG1_P2I, LCFG4_I2I, the user will have to download the SD sequence called "src5_ref__625.yuv" available at ftp://vqeg.its.bldrdoc.gov/SDTV/VQEG_PhaseI/TestSequences/ALL_625/. 
This sequence is in 4:2:2 format. Conversion to 4:2:0 format shall be made using the conversion tool 422_to_420fullres using the p option as following: 
--------------------------------------------------------------
">422_to_420perl.exe p src5_ref__625.yuv CANOA_720x576_25i.yuv"
--------------------------------------------------------------
The resulting CANOA_720x576_25i.yuv will then be used for the Interlaced validation scripts.


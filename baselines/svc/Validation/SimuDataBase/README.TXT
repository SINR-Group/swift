###############################################################################
# Copyright 2006 -- Thomson R&D France Snc
#                   Technology - Corporate Research
###############################################################################
# File          : README.TXT
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 0.0.4
################################################################################

###################################################################################
#PLEASE USE THE SPACES INSTEAD OF TABS when you write simulation and config files.# 
###################################################################################


The current directory contains various sub-directories each of them corresponding to a specific test-set.
--------------------------------------------------------------------------------------------------------
|
|-----Test-set1 (i.e. "Short_term")
|
|-----Test-set2 (i.e. "Long_term")
|
|-----Test-set3 (i.e. "AVC_Conformance")
|
|----- ...
|
|

Each test set is made up of several "Simulations". To each simulation is associated a directory. 
------------------------------------------------------------------------------------------------
For instance:
|
|-----Test-set1 (i.e. "Short_term")
|         |        
|         |-----Simulation1 (i.e. T1)
|         |
|         |-----Simulation2 (i.e. T3)
|         | 
|         |----- ....
|         |
|
|
|-----Test-set2 (i.e. "Long_term")
|
|-----Test-set3 (i.e. "AVC_Conformance")
|
|----- ...
|
|                                                      

A simulation directory contains at least a .TXT file and a "cfg" directory:
--------------------------------------------------------------------------- 
|
|-----Test-set1 (i.e. "Short_term")
|         |        
|         |-----Simulation1 (i.e. T1)
|         |         |
|         |         |----T1.txt : description of the Simulation : (MANDATORY)
|         |         |
|         |         |----cfg    : associated configuration files: (MANDATORY... most of the time)
|         |         |
|         |         |----[crop] : associated crop files (if needed) [OPTIONAL]
|         |         |
|         |         |----[orig] : Other YUV sequences (if needed)   [OPTIONAL] 
|         |         |
|         |         |----[str]  : bitstreams to decode (if needed)  [OPTIONAL]
|         |         |
|         |         |----[rec]  : reconstructed data (if needed)    [OPTIONAL]
|         |         |
|         |
|         |
|         |
|         |
|         |-----Simulation2 (i.e. T3)
|         | 
|         |----- ....
|         |
|
|
|-----Test-set2 (i.e. "Long_term")
|
|-----Test-set3 (i.e. "AVC_Conformance")
|
|----- ...
|
|        


A "Simulation" is made up of 2 parts:
-------------------------------------

-Encoding:
----------
 -It contains global encoding parameter as well specific parameters for each "Layer" to be encoded.
 -If required, a step of so-called "quality layer assignment" will be applied.
  
-Tests:
-------
 It contains various Extracting/decoding scenario to be run. The results will be automatically checked.



############################################################################################################
#                                 CONFIG FILES and NAMES CONVENTION	                                       #
############################################################################################################
Let us assume that the name of the simulation is "SimuName".
The users must respect the following rules when writing the config files. 

OutputFile      str/SimuName.str              # Bitstream file
LayerCfg        cfg/SimuName_Li.cfg           # Layer i configuration file
InputFile       orig/Simuname_176x144_15.yuv  # Input file "SimuName_widthxcolumn_framerate.yuv" of layer i
ReconFile       tmp/Simuname_rec_Li.yuv       # Reconstructed file of layer i
MotionInfoFile  mot/Simuname_Li.mot           # Motion Info File





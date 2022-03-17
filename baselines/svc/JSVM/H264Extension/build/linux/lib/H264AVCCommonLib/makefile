# the SOURCE definiton lets move your makefile to another position
CONFIG 		= LIBRARY

# set pathes to the correct directories
SRC_DIR		= ../../../../src/lib/H264AVCCommonLib
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin


USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# the library name
PRJ_NAME	 	= H264AVCCommonLib

# version information
MAJOR_VER		= 9
MINOR_VER		= 19
VER		= $(MAJOR_VER).$(MINOR_VER)

# defines to set
DEFS		= -DMSYS_LINUX

# set objects
OBJS          	= \
		$(OBJ_DIR)/CabacContextModel.o \
		$(OBJ_DIR)/CabacContextModel2DBuffer.o \
		$(OBJ_DIR)/DownConvert.o \
		$(OBJ_DIR)/Frame.o \
		$(OBJ_DIR)/H264AVCCommonLib.o \
		$(OBJ_DIR)/IntraPrediction.o \
		$(OBJ_DIR)/LoopFilter.o \
		$(OBJ_DIR)/MbData.o \
		$(OBJ_DIR)/MbDataAccess.o \
		$(OBJ_DIR)/MbDataCtrl.o \
		$(OBJ_DIR)/MbDataStruct.o \
		$(OBJ_DIR)/MbMvData.o \
		$(OBJ_DIR)/MbTransformCoeffs.o \
		$(OBJ_DIR)/MotionCompensation.o \
		$(OBJ_DIR)/MotionVectorCalculation.o \
		$(OBJ_DIR)/Mv.o \
		$(OBJ_DIR)/ParameterSetMng.o \
		$(OBJ_DIR)/PictureParameterSet.o \
		$(OBJ_DIR)/PocCalculator.o \
		$(OBJ_DIR)/Quantizer.o \
		$(OBJ_DIR)/QuarterPelFilter.o \
		$(OBJ_DIR)/ReconstructionBypass.o \
		$(OBJ_DIR)/ResizeParameters.o \
		$(OBJ_DIR)/SampleWeighting.o \
		$(OBJ_DIR)/SequenceParameterSet.o \
		$(OBJ_DIR)/ScalingMatrix.o \
		$(OBJ_DIR)/Sei.o \
		$(OBJ_DIR)/SliceHeader.o \
		$(OBJ_DIR)/SliceHeaderBase.o \
		$(OBJ_DIR)/Tables.o \
		$(OBJ_DIR)/TraceFile.o \
		$(OBJ_DIR)/Transform.o \
		$(OBJ_DIR)/YuvBufferCtrl.o \
		$(OBJ_DIR)/YuvMbBuffer.o \
		$(OBJ_DIR)/YuvPicBuffer.o \
		$(OBJ_DIR)/CFMO.o \
		$(OBJ_DIR)/Vui.o \
		$(OBJ_DIR)/Hrd.o \

LIBS		= -lpthread

DEBUG_LIBS		=
DEBUG32_LIBS	=
DEBUG64_LIBS	=
RELEASE_LIBS	=
RELEASE32_LIBS	=
RELEASE64_LIBS	=


# name of the base makefile
MAKE_FILE_NAME	= ../../common/makefile.base

# include the base makefile
include $(MAKE_FILE_NAME)

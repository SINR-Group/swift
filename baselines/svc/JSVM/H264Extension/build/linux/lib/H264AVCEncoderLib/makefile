# the SOURCE definiton lets move your makefile to another position
CONFIG 		= LIBRARY

# set pathes to the correct directories
SRC_DIR		= ../../../../src/lib/H264AVCEncoderLib
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin


USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# the library name
PRJ_NAME	 	= H264AVCEncoderLib

# version information
MAJOR_VER		= 9
MINOR_VER		= 19
VER		= $(MAJOR_VER).$(MINOR_VER)

# defines to set
DEFS		= -DMSYS_LINUX -DSHARP_AVC_REWRITE_OUTPUT

# set objects
OBJS          	= \
		$(OBJ_DIR)/BitCounter.o \
		$(OBJ_DIR)/BitWriteBuffer.o \
		$(OBJ_DIR)/CabacWriter.o \
		$(OBJ_DIR)/CabaEncoder.o \
		$(OBJ_DIR)/CodingParameter.o \
		$(OBJ_DIR)/ControlMngH264AVCEncoder.o \
		$(OBJ_DIR)/CreaterH264AVCEncoder.o \
		$(OBJ_DIR)/Distortion.o \
		$(OBJ_DIR)/GOPEncoder.o \
		$(OBJ_DIR)/H264AVCEncoder.o \
		$(OBJ_DIR)/H264AVCEncoderLib.o \
		$(OBJ_DIR)/InputPicBuffer.o \
		$(OBJ_DIR)/IntraPredictionSearch.o \
		$(OBJ_DIR)/MbCoder.o \
		$(OBJ_DIR)/MbEncoder.o \
		$(OBJ_DIR)/MbTempData.o \
		$(OBJ_DIR)/MotionEstimation.o \
		$(OBJ_DIR)/MotionEstimationCost.o \
		$(OBJ_DIR)/MotionEstimationQuarterPel.o \
		$(OBJ_DIR)/NalUnitEncoder.o \
		$(OBJ_DIR)/PicEncoder.o \
		$(OBJ_DIR)/RateDistortion.o \
		$(OBJ_DIR)/RecPicBuffer.o \
		$(OBJ_DIR)/SequenceStructure.o \
		$(OBJ_DIR)/SliceEncoder.o \
		$(OBJ_DIR)/UvlcWriter.o \
		$(OBJ_DIR)/Scheduler.o \
                        $(OBJ_DIR)/RateCtlBase.o \
                        $(OBJ_DIR)/RateCtlQuadratic.o \


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

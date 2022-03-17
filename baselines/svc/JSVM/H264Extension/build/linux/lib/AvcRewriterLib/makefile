# the SOURCE definiton lets move your makefile to another position
CONFIG 		= LIBRARY

# set pathes to the correct directories
SRC_DIR		= ../../../../src/lib/H264AVCDecoderLib
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin

USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# the library name
PRJ_NAME	 	= AvcRewriterLib

# version information
MAJOR_VER		= 9
MINOR_VER		= 19
VER		= $(MAJOR_VER).$(MINOR_VER)

# defines to set
DEFS		= -DMSYS_LINUX -DSHARP_AVC_REWRITE_OUTPUT

# set objects
OBJS          	= \
		$(OBJ_DIR)/BitReadBuffer.o \
		$(OBJ_DIR)/CabacReader.o \
		$(OBJ_DIR)/CabaDecoder.o \
		$(OBJ_DIR)/ControlMngH264AVCDecoder.o \
		$(OBJ_DIR)/CreaterH264AVCDecoder.o \
		$(OBJ_DIR)/GOPDecoder.o \
		$(OBJ_DIR)/H264AVCDecoder.o \
		$(OBJ_DIR)/H264AVCDecoderLib.o \
		$(OBJ_DIR)/MbDecoder.o \
		$(OBJ_DIR)/MbParser.o \
		$(OBJ_DIR)/NalUnitParser.o \
		$(OBJ_DIR)/SliceDecoder.o \
		$(OBJ_DIR)/SliceReader.o \
		$(OBJ_DIR)/UvlcReader.o \


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

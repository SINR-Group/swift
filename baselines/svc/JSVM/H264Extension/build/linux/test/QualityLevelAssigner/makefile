# the SOURCE definiton lets you move your makefile to another position
CONFIG 		= CONSOLE

# set directories to your wanted values
SRC_DIR		= ../../../../src/test/QualityLevelAssigner
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin

USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# set executable name
PRJ_NAME		= QualityLevelAssigner

# defines to set
DEFS		= -DMSYS_LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DMSYS_UNIX_LARGEFILE

# set objects
OBJS          	= \
		$(OBJ_DIR)/QualityLevelAssigner.o \
		$(OBJ_DIR)/QualityLevel.o \
		$(OBJ_DIR)/QualityLevelEstimation.o \
		$(OBJ_DIR)/QualityLevelParameter.o \

# set libs to link with
LIBS		= -ldl -lpthread

DEBUG_LIBS		= -lH264AVCDecoderLibStaticd   -lH264AVCCommonLibStaticd   -lH264AVCVideoIoLibStaticd   -lH264AVCEncoderLibStaticd
DEBUG32_LIBS	= -lH264AVCDecoderLibStatic32d -lH264AVCCommonLibStatic32d -lH264AVCVideoIoLibStatic32d -lH264AVCEncoderLibStatic32d
DEBUG64_LIBS	= -lH264AVCDecoderLibStatic64d -lH264AVCCommonLibStatic64d -lH264AVCVideoIoLibStatic64d -lH264AVCEncoderLibStatic64d

DEBUG_PREREQS	= $(LIB_DIR)/libH264AVCDecoderLibStaticd.a   $(LIB_DIR)/libH264AVCCommonLibStaticd.a   $(LIB_DIR)/libH264AVCVideoIoLibStaticd.a   $(LIB_DIR)/libH264AVCEncoderLibStaticd.a
DEBUG32_PREREQS	= $(LIB_DIR)/libH264AVCDecoderLibStatic32d.a $(LIB_DIR)/libH264AVCCommonLibStatic32d.a $(LIB_DIR)/libH264AVCVideoIoLibStatic32d.a $(LIB_DIR)/libH264AVCEncoderLibStatic32d.a
DEBUG64_PREREQS	= $(LIB_DIR)/libH264AVCDecoderLibStatic64d.a $(LIB_DIR)/libH264AVCCommonLibStatic64d.a $(LIB_DIR)/libH264AVCVideoIoLibStatic64d.a $(LIB_DIR)/libH264AVCEncoderLibStatic64d.a

RELEASE_LIBS	= -lH264AVCDecoderLibStatic   -lH264AVCCommonLibStatic   -lH264AVCVideoIoLibStatic   -lH264AVCEncoderLibStatic
RELEASE32_LIBS	= -lH264AVCDecoderLibStatic32 -lH264AVCCommonLibStatic32 -lH264AVCVideoIoLibStatic32 -lH264AVCEncoderLibStatic32
RELEASE64_LIBS	= -lH264AVCDecoderLibStatic64 -lH264AVCCommonLibStatic64 -lH264AVCVideoIoLibStatic64 -lH264AVCEncoderLibStatic64

RELEASE_PREREQS	= $(LIB_DIR)/libH264AVCDecoderLibStatic.a   $(LIB_DIR)/libH264AVCCommonLibStatic.a   $(LIB_DIR)/libH264AVCVideoIoLibStatic.a   $(LIB_DIR)/libH264AVCEncoderLibStatic.a
RELEASE32_PREREQS	= $(LIB_DIR)/libH264AVCDecoderLibStatic32.a $(LIB_DIR)/libH264AVCCommonLibStatic32.a $(LIB_DIR)/libH264AVCVideoIoLibStatic32.a $(LIB_DIR)/libH264AVCEncoderLibStatic32.a
RELEASE64_PREREQS	= $(LIB_DIR)/libH264AVCDecoderLibStatic64.a $(LIB_DIR)/libH264AVCCommonLibStatic64.a $(LIB_DIR)/libH264AVCVideoIoLibStatic64.a $(LIB_DIR)/libH264AVCEncoderLibStatic64.a


# name of the base makefile
MAKE_FILE_NAME	= ../../common/makefile.base

# include the base makefile
include $(MAKE_FILE_NAME)

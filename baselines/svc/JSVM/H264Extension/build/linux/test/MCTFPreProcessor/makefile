# the SOURCE definiton lets you move your makefile to another position
CONFIG 		= CONSOLE

# set directories to your wanted values
SRC_DIR		= ../../../../src/test/MCTFPreProcessor
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin

USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# set executable name
PRJ_NAME		= MCTFPreProcessor

# defines to set
DEFS		= -DMSYS_LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DMSYS_UNIX_LARGEFILE

# set objects
OBJS          	= \
		$(OBJ_DIR)/MCTF.o \
		$(OBJ_DIR)/MCTFPreProcessor.o \
		$(OBJ_DIR)/MCTFPreProcessorTest.o \
		$(OBJ_DIR)/PreProcessor.o \
		$(OBJ_DIR)/PreProcessorParameter.o \

# set libs to link with
LIBS		= -ldl -lpthread

DEBUG_LIBS		= -lH264AVCVideoIoLibStaticd   -lH264AVCEncoderLibStaticd   -lH264AVCCommonLibStaticd
DEBUG32_LIBS	= -lH264AVCVideoIoLibStatic32d -lH264AVCEncoderLibStatic32d -lH264AVCCommonLibStatic32d
DEBUG64_LIBS	= -lH264AVCVideoIoLibStatic64d -lH264AVCEncoderLibStatic64d -lH264AVCCommonLibStatic64d

DEBUG_PREREQS	= $(LIB_DIR)/libH264AVCVideoIoLibStaticd.a   $(LIB_DIR)/libH264AVCEncoderLibStaticd.a   $(LIB_DIR)/libH264AVCCommonLibStaticd.a
DEBUG32_PREREQS	= $(LIB_DIR)/libH264AVCVideoIoLibStatic32d.a $(LIB_DIR)/libH264AVCEncoderLibStatic32d.a $(LIB_DIR)/libH264AVCCommonLibStatic32d.a
DEBUG64_PREREQS	= $(LIB_DIR)/libH264AVCVideoIoLibStatic64d.a $(LIB_DIR)/libH264AVCEncoderLibStatic64d.a $(LIB_DIR)/libH264AVCCommonLibStatic64d.a

RELEASE_LIBS	= -lH264AVCVideoIoLibStatic   -lH264AVCEncoderLibStatic   -lH264AVCCommonLibStatic
RELEASE32_LIBS	= -lH264AVCVideoIoLibStatic32 -lH264AVCEncoderLibStatic32 -lH264AVCCommonLibStatic32
RELEASE64_LIBS	= -lH264AVCVideoIoLibStatic64 -lH264AVCEncoderLibStatic64 -lH264AVCCommonLibStatic64

RELEASE_PREREQS	= $(LIB_DIR)/libH264AVCVideoIoLibStatic.a   $(LIB_DIR)/libH264AVCEncoderLibStatic.a    $(LIB_DIR)/libH264AVCCommonLibStatic.a
RELEASE32_PREREQS	= $(LIB_DIR)/libH264AVCVideoIoLibStatic32.a $(LIB_DIR)/libH264AVCEncoderLibStatic32.a  $(LIB_DIR)/libH264AVCCommonLibStatic32.a
RELEASE64_PREREQS	= $(LIB_DIR)/libH264AVCVideoIoLibStatic64.a $(LIB_DIR)/libH264AVCEncoderLibStatic64.a  $(LIB_DIR)/libH264AVCCommonLibStatic64.a


# name of the base makefile
MAKE_FILE_NAME	= ../../common/makefile.base

# include the base makefile
include $(MAKE_FILE_NAME)

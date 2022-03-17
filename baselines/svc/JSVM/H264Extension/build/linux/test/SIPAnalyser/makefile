# the SOURCE definiton lets you move your makefile to another position
CONFIG 		= CONSOLE

# set directories to your wanted values
SRC_DIR		= ../../../../src/test/SIPAnalyser
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin

USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# set executable name
PRJ_NAME		= SIPAnalyser

# defines to set
DEFS		= -DMSYS_LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DMSYS_UNIX_LARGEFILE

# set objects
OBJS          	= \
		$(OBJ_DIR)/SIPAnalyser.o \
		$(OBJ_DIR)/SIPParameters.o \
		$(OBJ_DIR)/SIP.o \

# set libs to link with
LIBS		= -ldl -lpthread

DEBUG_LIBS		= -lH264AVCCommonLibStaticd
DEBUG32_LIBS	= -lH264AVCCommonLibStatic32d
DEBUG64_LIBS	= -lH264AVCCommonLibStatic64d

DEBUG_PREREQS	= $(LIB_DIR)/libH264AVCCommonLibStaticd.a
DEBUG32_PREREQS	= $(LIB_DIR)/libH264AVCCommonLibStatic32d.a
DEBUG64_PREREQS	= $(LIB_DIR)/libH264AVCCommonLibStatic64d.a

RELEASE_LIBS	= -lH264AVCCommonLibStatic
RELEASE32_LIBS	= -lH264AVCCommonLibStatic32
RELEASE64_LIBS	= -lH264AVCCommonLibStatic64

RELEASE_PREREQS	= $(LIB_DIR)/libH264AVCCommonLibStatic.a
RELEASE32_PREREQS	= $(LIB_DIR)/libH264AVCCommonLibStatic32.a
RELEASE64_PREREQS	= $(LIB_DIR)/libH264AVCCommonLibStatic64.a


# name of the base makefile
MAKE_FILE_NAME	= ../../common/makefile.base

# include the base makefile
include $(MAKE_FILE_NAME)

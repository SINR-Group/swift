# the SOURCE definiton lets move your makefile to another position
CONFIG 		= LIBRARY

# set pathes to the correct directories
SRC_DIR		= ../../../../src/lib/H264AVCVideoIoLib
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin


USER_INC_DIRS	= -I$(SRC_DIR)
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# the library name
PRJ_NAME	 	= H264AVCVideoIoLib

# version information
MAJOR_VER		= 9
MINOR_VER		= 19
VER		= $(MAJOR_VER).$(MINOR_VER)

# defines to set
DEFS		= -DMSYS_LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DMSYS_UNIX_LARGEFILE

# set objects
OBJS          	= \
		$(OBJ_DIR)/H264AVCVideoIoLib.o \
		$(OBJ_DIR)/LargeFile.o \
		$(OBJ_DIR)/ReadBitstreamFile.o \
		$(OBJ_DIR)/ReadYuvFile.o \
		$(OBJ_DIR)/WriteBitstreamToFile.o \
		$(OBJ_DIR)/WriteYuvToFile.o \

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

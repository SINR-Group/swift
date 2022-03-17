# the SOURCE definiton lets you move your makefile to another position
CONFIG 		= CONSOLE

# set directories to your wanted values
SRC_DIR		= ../../../../src/test/FixedQPEncoder
INC_DIR		= ../../../../include
LIB_DIR		= ../../../../../../lib
BIN_DIR		= ../../../../../../bin

USER_INC_DIRS	= -I$(SRC_DIR) 
USER_LIB_DIRS	=

# intermediate directory for object files
OBJ_DIR		= ./objects

# set executable name
PRJ_NAME		= FixedQPEncoder

# defines to set
DEFS		= -DMSYS_LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DMSYS_UNIX_LARGEFILE

# set objects
OBJS          	= \
		$(OBJ_DIR)/FixedQPEncoder.o \

# set libs to link with
LIBS		= -ldl -lpthread

DEBUG_LIBS		= 
DEBUG32_LIBS	= 
DEBUG64_LIBS	= 

DEBUG_PREREQS	= 
DEBUG32_PREREQS	= 
DEBUG64_PREREQS	= 

RELEASE_LIBS	= 
RELEASE32_LIBS	= 
RELEASE64_LIBS	= 

RELEASE_PREREQS	= 
RELEASE32_PREREQS	= 
RELEASE64_PREREQS	= 


# name of the base makefile
MAKE_FILE_NAME	= ../../common/makefile.base

# include the base makefile
include $(MAKE_FILE_NAME)

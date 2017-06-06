.PHONY: all clean help
all: 

# Some important on-off settings. You can not be too careful about them.
DEBUG       	:= y
# Flag of generate a dynamic lib or a static lib: y means yes. If the target is a excutable file, it should be blank!
GEN_LIBS    	:= y
# Flag of generate a dynamic lib: y means yes. It should be blank unless you want to generate a dynamic lib!
GEN_DYN_LIB		:= y
# The name of target bin file.Please let it be blank unless the target is a excutable file. 
EXCUTE_BIN		:= 
# Name of the static lib. It should be blank unless the target is a static lib, then the GEN_LIBS is y and GEN_DYN_LIB is blank.
# STATIC_LIBS		:= libsrcpbl.a
# Name of the dynamic lib. It should be blank unless the target is a dynamic lib, then the GEN_LIBS is y and GEN_DYN_LIB is y.
DYNAMIC_LIBS 	:= libsrcpbl_bill.so

# Environment settings. The value of PROJECT_DIR shoule be set in the *nix system as the the absolute dir path of your project.
# PROJECT_DIR	:= /home/test_usr
#CURDIR    		:= $(PROJECT_DIR)/src/test
CURDIR     		:= $(shell pwd)
SYBASE_DIR		:= /opt/sybase/OCS-16_0
SYBASE_BIN_DIR	:= $(SYBASE_DIR)/bin
SYBASE_LIB_DIR	:= $(SYBASE_DIR)/lib
SYBASE_INC_DIR	:= $(SYBASE_DIR)/include

# Cross compile tools defined. You needn't modify these vars below generally.
AS    	:= as
LD    	:= ld
CC    	:= gcc
CXX   	:= g++
CPP   	:= $(CC) -E
AR      := ar rcs
NM      := nm
STRIP   := strip
RANLIB 	:= ranlib
STD_OPT	:= -D_GNU_SOURCE 
CC    	+= -std=gnu99 $(STD_OPT)
CXX    	+= $(STD_OPT)
PC    	:= $(SYBASE_DIR)/bin/cpre64

# *nix system tools defined. You needn't modify these vars below generally.
CD		:= cd
CP		:= cp
SED		:= sed
FIND	:= find
MKDIR 	:= mkdir -p
XARGS	:= xargs
MV    	:= mv
RM    	:= rm -rf


# Complie and link variables. LD_LIBS means the dynamic or static library needed for the object file.
CFLAGS      := $(if $(DEBUG),-g -Wall, -O2 -Wall)
CFLAGS      += $(if $(GEN_DYN_LIB), $(addprefix -fPIC -I ,$(sort $(dir $(SRC_H)))), $(addprefix -I ,$(sort $(dir $(SRC_H)))))
CXXFLAGS    = $(CFLAGS)
LDFLAGS     := 
LD_LIB_DIR  := -L$(SYBASE_LIB_DIR)
LD_LIBS     := -lsybct64 -lsybtcl64 -lsybcs64 -lsybcomn64 -lsybintl64 -lsybunic64  -rdynamic -ldl -lnsl -lm
XLD_FLG     := -Xlinker "-(" $(LDFLAGS) -Xlinker "-)"



all: $(DYNAMIC_LIBS)

OBJS_TRG = spp_query_1.o sybesql.o

$(DYNAMIC_LIBS): $(OBJS_TRG)
	$(CC) -shared -fPIC -o $@ $^


# implicit
 
.SUFFIXES:
.SUFFIXES: .sqc .cpp .c .o


.sqc.o:
	echo "[$(PC)] Compiling sqc [$@] ..."
	$(PC) -y -I. -m $<
	echo "$(CC) Compiling sqc [$@] ..."
	$(CC) -fPIC -m64  -g -DSYB_LP64 -I. -I$(SYBASE_INC_DIR) -c $*.c $(SYBASE_INC_DIR)/sybesql.c $(LD_LIB_DIR) $(LD_LIBS)
	@$(RM) $*.c

.cpp.o:
	echo "Compiling cpp [$@] ..."
	@$(RM) $*.o
	$(CXX) -fPIC -o $@ -c $<


.c.o:
	echo "Compiling c [$@] ..."
	@$(RM) $*.o
	$(CC) -fPIC -o $@ -c $<


clean: 
	-$(FIND) $(CURDIR) -name "*.o" -o -name "*.d" | $(XARGS) $(RM)
	-$(RM) $(ULT_BIN) $(ULT_LIBS) ${DYNAMIC_LIBS} spp_query_1.c

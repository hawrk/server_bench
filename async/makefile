COMM = ../comm
CONFBASE = ../comm/tconfbase
TBASE = ../comm/tbase
TSOCKCOMMU = ../comm/tbase/tsockcommu
PROXY= ../proxy
WORKER=../worker
ASYNC=../async
LIB_PATH=../../bin/lib/


ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

CFLAGS += -O2 -g -ggdb -Wall -D_GNU_SOURCE -Wno-write-strings -Werror 
CFLAGS += -pipe -fno-ident -fPIC -shared -z defs
INC += -I./ -I$(COMM) -I$(CONFBASE) -I$(TBASE) -I$(TBASE)/tsockcommu -I$(ASYNC)
LIB += -lpthread -ldl -rdynamic -L$(COMM) -L$(ASYNC)/ -lcommon -Wl,-rpath,$(LIB_PATH)
TARGET=libasync_epoll.so
all : $(TARGET)
OBJ = client_unit.o \
	general_session.o \
	tm_session.o \
	session_mgr.o \
	global.o \
	interface.o \
	spp_async_interface.o


$(TARGET):$(OBJ)
	@echo -e  Linking $@$
	@$(CXX) -o $@ $^ -shared $(CFLAGS) $(LIB) 
	#install *.so ../../bin/lib/

%.o: %.cpp
	@echo -e Compiling $<
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC)   
%.o: %.c
	@echo -e Compiling $<
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC)  
clean:
	@rm -f $(OBJ) $(TARGET)


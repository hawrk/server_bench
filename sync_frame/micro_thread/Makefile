########MAKEFILE##########
DEBUG= -g

BINARY = libmt.so

# Comment the following line if you are not using the gnu c compiler
#C_ARGS = -Wall -g -fPIC -D_DEBUG
C_ARGS = -Wall -g -fPIC -O1
#.SUFFIXES: .o .cpp

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif
# You might have to change this if your c compiler is not cc
CC = g++

# You shouldn't need to make any more changes below this line.
INCCOMM=-I../valgrind

#.c.o:
#	$(CC) -c $*.c

all:	$(BINARY) 

%.o: %.cpp
	@echo -e Compiling $< ...
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) 

%.o: %.c
	@echo -e Compiling $< ...
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) 

%.o: %.S
	@echo -e Compiling $<$ ...
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) 

clean:
	@rm -f $(BINARY)  *.o 


LIB_OBJ = micro_thread.o epoll_proxy.o arch_ctx.o mt_session.o mt_notify.o mt_action.o mt_mbuf_pool.o mt_api.o\
	 mt_connection.o mt_concurrent.o mt_sys_hook.o heap_timer.o  mt_cache.o  mt_net.o

libmt.a: $(LIB_OBJ)
	@echo -e  Linking $@ ...
	@-rm -f $@ 
	@ar crs  $@  $^  
	@chmod +x $@ 

libmt.so: $(LIB_OBJ)
	@echo -e  Linking $@ ...
	@$(CC) -o $@ $^ -shared $(C_ARGS) -Wl,-z,interpose 


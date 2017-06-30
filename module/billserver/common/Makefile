#------------------------
#----hawrk 2017.04.18----
#------------------------
INC_HOME	:= ../../Base/include
LIB_HOME	:= ../../Base/lib64
BASE_COMM   := ../../Base/Comm
#objects
LIBRARY		:= ../../Base/library
#busi
OPENSSL_ROOT= ../../Base/third_party/openssl
CURL_ROOT=../../Base/third_party/curl
TINYXML_ROOT=../../Base/third_party/tinyxml2
RAPIDJSON_ROOT=../../Base/third_party/rapidjson
URI_ROOT=../../Base/third_party/network-uri
BOOST_ROOT=../../Base/third_party/boost
SSH2_ROOT=/usr/local/services/ssh
AMQP=./amqp
RABBITMQ_ROOT=../../Base/third_party/rabbitmq

########## compiler flags ##########
INCS		:= -I./ -I../business -I$(INC_HOME) -I$(INC_HOME)/xml/  -I$(BASE_COMM)\
           -I$(CURL_ROOT)/include \
	       -I$(OPENSSL_ROOT)/include \
	       -I$(TINYXML_ROOT)/tinyxml2 \
           -I$(RAPIDJSON_ROOT)/include \
           -I$(URI_ROOT)/include \
		   -I$(SSH2_ROOT)/include \
           -I$(BOOST_ROOT) 
           
DEFS		:= -D_GNU_SOURCE -D_REENTRANT -D_OS_LINUX_ -D_COMPILER_CUR_=_COMPILER_GNU_GCC_4_1_2_  -D_NO_COMPILE_POLICY_MESSAGE_

LIBS		:= -L$(RABBITMQ_ROOT)/lib -lrabbitmq\

CC			:= gcc
CFLAGS		:= -pipe  -O2 -Wall $(INCS) $(DEFS) -fPIC
CXX			:= g++
CXXFLAGS	:= $(CFLAGS)
AR = ar -rv

LIB_COMM_OBJS = $(patsubst ./%.cpp, ./%.o, $(wildcard ./*.cpp))

%.o : %.c
	$(CC) -c $(CFLAGS)  -o $@ $< $(LIBS)
	
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS)  -o $@ $<

LIB_OBJS = $(LIB_COMM_OBJS)

TARGETS = libbillcomm.a

all: $(TARGETS) 

$(TARGETS): $(LIB_OBJS)
	$(AR) $@ $^
clean:
	rm -f $(LIB_COMM_OBJS)
	rm -f $(TARGETS)
	
	
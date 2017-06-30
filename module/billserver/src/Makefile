INC_HOME	:= ../../Base/include
LIB_HOME	:= ../../Base/lib64
PAY_LIB_HOME	:= ../lib
#objects
BASE		:= ../../Base
COMM		:= ../../Base/Comm
LIBRARY		:= ../../Base/library
CPS_DIR		:= ../../OrderServer/cps
DB_DIR      := ../../server.filter/src

#src depend on
BILL_COMM   := ../common
SRC_INC		:= ../business


#for dev envirement only
INSTALLIMG=/usr/local/services/spp_speedpos_bill-2.0/client/speedpos_bill/lib/


#busi
OPENSSL_ROOT= ../../Base/third_party/openssl
CURL_ROOT=../../Base/third_party/curl
TINYXML_ROOT=../../Base/third_party/tinyxml2
RAPIDJSON_ROOT=../../Base/third_party/rapidjson
URI_ROOT=../../Base/third_party/network-uri
HIREDIS_ROOT=../../Base/third_party/hiredis
BOOST_ROOT=../../Base/third_party/boost
SSH2_ROOT=../../Base/third_party/ssh
COMM_ROOT=../../Base/Comm
RABBITMQ_ROOT=../../Base/third_party/rabbitmq

#plugin info
LIBNAME = libbillproc
VERSION = 2.0
TARGET		:= $(LIBNAME)-$(VERSION).so

#GCC_VER		:= 4.1
GCC_VERSION	:= $(shell gcc -dumpversion)
GCC_MACHINE	:= $(shell gcc -dumpmachine)

########## compiler flags ##########
INCS		:= -I. -I.. -I../../ -I$(SRC_INC) -I$(BILL_COMM) -I$(COMM) -I$(DB_DIR) -I$(INC_HOME) -I$(BASE)/fw/ -I$(INC_HOME)/xml/  -I$(COMM)/business/ -I$(COMM)/ttc_engine/  \
           -I$(CURL_ROOT)/include \
	       -I$(OPENSSL_ROOT)/include \
	       -I$(TINYXML_ROOT)/tinyxml2 \
           -I$(RAPIDJSON_ROOT)/include \
           -I$(URI_ROOT)/include \
           -I$(BASE)/tars_util/include \
		   -I$(HIREDIS_ROOT)/include \
		   -I$(SSH2_ROOT)/include \
           -I$(BOOST_ROOT) \
          
		   
		   
DEFS		:= -D_GNU_SOURCE -D_REENTRANT -D_OS_LINUX_ -D_COMPILER_CUR_=_COMPILER_GNU_GCC_4_1_2_  -D_NO_COMPILE_POLICY_MESSAGE_

ifdef debug
DEFS		:= ${DEFS} -DDEBUG -pg
endif

CC			:= gcc
CFLAGS		:= -pipe  -O2 -Wall $(INCS) $(DEFS) -fPIC
CXX			:= g++
CXXFLAGS	:= $(CFLAGS)

LDFLAGS	:= -shared -fPIC

OBJS := order_spp_main.o CBillBusiConfig.o  CSpeedPosConfig.o \
		CSpeedPosServer.o openapi_client.o  CSppClient.o \
		BankFactory.o CBillConstrastBase.o CBillConstrastCommon.o \
		CProcBillContrastTask.o CProcSettleCallback.o  CPayTransactionFlowDao.o \
		CProcPushSysBillTask.o  CBillConstrastSPDB.o CBillDownLoadManage.o CBillBatchManage.o\
		CBillSummaryQry.o CBillAbnormalQry.o CSettleQryTask.o CSettleAcctinfoModifyTask.o\
		CAgentPayBase.o CRabbitMQClient.o CAgentPayBillDealTask.o CAgentPayAbnormalQryTask.o CAgentPayBillLogQryTask.o\
		$(COMM)/md5c.o $(COMM)/log/clog.o\
		$(COMM)/config/HttpConfig.o $(COMM)/config/ServerConfig.o\
		$(COMM)/svr/SvrClient.o 
		
#-include $(OBJS:.o=.d)

LIBS		:= -L$(LIB_HOME) -lxmlParser -lqos_client -lttc -lmsglog -ldnscli -lrt -lpthread -liconv -lcomm\
				-lpcre -ldcapi  -ltinyxml -lcrypto -lssl -ltarsutil -lmysqlclient\
				-L$(BILL_COMM) -lbillcomm \
               $(CURL_ROOT)/lib/libcurl.a \
	           $(TINYXML_ROOT)/lib64/libtinyxml2.a \
			   -L$(SSH2_ROOT)/lib -lssh2 \
			   -L$(BOOST_ROOT)/lib/ -lboost_context -lboost_thread -lboost_system \
               -L$(RABBITMQ_ROOT)/lib -lrabbitmq\

$(TARGET) : $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(_NAME) : $(TARGET) #dummy for ../../Makefile
	

order_notify : 
	cd ./notify/;${MAKE} all

compile_test : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS) 

#Compiling C objects
%.o : %.c
	$(CC) -c $(CFLAGS)  -o $@ $<

#Compiling C++ objects
%.o : %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $<
	$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS)   -o $@ $<

all: $(TARGET) $(TARGET2) order_notify
	

install:
	cp -f $(TARGET) $(INSTALLIMG)

uninstall:
	rm -f $(INSTALLIMG)

clean:
	rm -f $(TARGET) $(TARGET2)
	rm -f $(OBJS)
	

CC = g++

TOP_SRCDIR = ..

IO_DIR = $(TOP_SRCDIR)/io
CO_DIR = $(TOP_SRCDIR)/common
DB_DIR = $(TOP_SRCDIR)/storage
MD_DIR = ./model

LIB_DIR = lib

INCLUDES = -I. -I$(TOP_SRCDIR) -I$(IO_DIR) -I$(CO_DIR) -I$(DB_DIR) -I$(MD_DIR)

DEFINES = -D_REENTRANT_  -DUSE_WDB

LIBS = $(LIB_DIR)/libjson_4.1.2.a

.cpp.o:
	$(CC) $(INCLUDES) $(DEFINES) -g -c $< -o $@

.c.o:
	$(CC) -g -c $< -o $@


OUTEROBJS = ../common/log2_m.o ../common/timer_m.o ../common/itimer_m.o ../common/octets_m.o ../common/conf_m.o ../common/thread_m.o  \
			../io/pollio_m.o ../io/protocol_m.o ../io/security_m.o 
OBJS = ChatMessage.o ControlProtocol.o $(MD_DIR)/User.o $(MD_DIR)/Friend.o $(MD_DIR)/Message.o $(MD_DIR)/FriendRequest.o 

LDFLAGS = -pthread

EXES = YChat YChatServer

all : $(EXES)

YChat: $(OBJS) YChat.o kbhit.o
	g++ $(LDFLAGS) $(OUTEROBJS) $^ $(LIBS) -o $@

YChatServer: $(OBJS) $(DB_DIR)/storage.o YChatDB.o YChatServer.o
	g++ $(LDFLAGS) $(OUTEROBJS) $^ $(LIBS) -o $@

test: model/Friend.o model/User.o test.o 
	g++ $^ $(LIBS) -o $@

clean: 
	rm -vf *.o

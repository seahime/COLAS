ALL = process 
CC=gcc -std=gnu99 -g  -DDEBUG

CFLAGS= -Iutilities/ -Iabd/ -Icodes/ -Isodaw  -IZMQ/include 
LDFLAGS= -lm -LZMQ/zmqlibs  -LZMQ/czmqlibs  -lzmq -lczmq  -Labd/ -labd -Lcodes/ -lrlnc_rs -Lsodaw -lsodaw -Wimplicit-function-declaration -fPIC -Wall 


all: $(ALL)

TARGET= process
TARGET1= processc

SRCS = daemons/params.go daemons/logger.go daemons/controllerRoutes.go daemons/getParamRoutes.go  daemons/setParamRoutes.go daemons/httpServer.go daemons/routes.go daemons/controller.go daemons/utils.go daemons/writer.go daemons/reader.go daemons/server.go daemons/utilities.go

CSRCS = abd/abd_client.c abd/abd_reader.c abd/abd_writer.c abd/abd_server.c abd/abd_client.h abd/abd_reader.h abd/abd_writer.h  abd/abd_server.h  abd/algo_utils.h abd/algo_utils.c

process: $(TARGET) $(SRCS)  #$(CSRCS)
	go fmt $(SRCS)
	go build -x -o process $(TARGET).go

all=CODES ABD SODAW $(TARGET)

ABD:
	@echo "Building ABD"
	make -C abd clean
	make -C abd
	cp ./abd/libabd.so /lib64/

SODAW:
	@echo "Building SODAW"
	make -C sodaw clean
	make -C sodaw

CODES:
	@echo "Building CODES"
	make -C codes clean
	make -C codes


rebuildc: remove buildc
	make -C codes clean
	make -C abd clean
	make -C sodaw clean

buildc: process.c utilities/helpers.c utilities/helpers.h 
	make -C codes 
	make -C abd 
	make -C sodaw
	$(CC) $(CFLAGS) utilities/helpers.c  process.c  -o $(TARGET1)  $(LDFLAGS)


rebuild: remove build

.PHONY:remove
remove:
	make -C codes clean
	make -C abd clean
	make -C sodaw clean
	rm -rf  $(TARGET1)
	rm -rf  $(TARGET)

build:
	make -C codes 
	make -C abd 
	make -C sodaw
	go build -x -o $(TARGET) process.go

clean:
	rm -rf logs $(TARGET) $(TARGET1) */*.o

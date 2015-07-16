######################################################################################
#Author                   Weilin.Chen (QQ:532776869, GitHub: https://github.com/4Second2None)
#Date                     2015-05-31 21:18:00
#Note                     Compile some programs include testcase , library of state_threads and library st_websocket (server and client)
######################################################################################


THIRD_LIBRARY = libst.a
SERVEROBJ = $(patsubst %.c, %.o, $(wildcard ./server/*.c))
CLIENTOBJ = $(patsubst %.c, %.o, $(wildcard ./client/*.c))

SERTESTOBJ = $(patsubst %.c, %.o, $(wildcard ./server/test/*.c))
CLITESTOBJ = $(patsubst %.c, %.o, $(wildcard ./client/test/*.c))

LOCAL_LDFLAG = -ldl

LOCAL_FLAG = -O3 -fPIC -shared -I ./state-threads-master/obj -I ./

CLITEST_LDFLAG =  ./libst.a ./libcws.a -I ./ -I ./client -I ./state-threads-master/obj
SERTEST_LDFLAG =  ./libst.a ./libsws.a -I ./ -I ./server -I ./state-threads-master/obj
#TEST_LDFLAG = -L. -lcws  -lst

SHARED_SERTARGET_FILE = libsws.so
SHARED_CLITARGET_FILE = libcws.so
STATIC_SERTARGET_FILE = libsws.a
STATIC_CLITARGET_FILE = libcws.a
SERTEST_TARGET_FILE = st_server
CLITEST_TARGET_FILE = st_client


CC = gcc

all:$(THIRD_LIBRARY) $(STATIC_SERTARGET_FILE) $(SHARED_SERTARGET_FILE) $(SERTEST_TARGET_FILE) $(STATIC_CLITARGET_FILE) $(SHARED_CLITARGET_FILE) $(CLITEST_TARGET_FILE)


$(THIRD_LIBRARY):
	cd ./state-threads-master/ && make linux-debug
	cp ./state-threads-master/obj/libst.a ./


$(filter %.o,$(CLIENTOBJ)):%.o:%.c
	$(CC) -c $(LOCAL_FLAG) $(LOCAL_LDFLAG) $< -o $@

$(filter %.o,$(SERVEROBJ)):%.o:%.c
	$(CC) -c $(LOCAL_FLAG) $(LOCAL_LDFLAG) $< -o $@

$(filter %.o,$(CLITESTOBJ)):%.o:%.c
	$(CC) -c -O3 $(CLITEST_LDFLAG) $< -o $@

$(filter %.o,$(SERTESTOBJ)):%.o:%.c
	$(CC) -c -O3 $(SERTEST_LDFLAG) $< -o $@

$(SHARED_CLITARGET_FILE):$(CLIENTOBJ)
	$(CC) -o $(SHARED_CLITARGET_FILE) $(CLIENTOBJ) $(LOCAL_FLAG) $(LOCAL_LDFLAG)

$(STATIC_CLITARGET_FILE):$(CLIENTOBJ)
	ar cr $(STATIC_CLITARGET_FILE) $(CLIENTOBJ)

$(SHARED_SERTARGET_FILE):$(SERVEROBJ)
	$(CC) -o $(SHARED_SERTARGET_FILE) $(SERVEROBJ) $(LOCAL_FLAG) $(LOCAL_LDFLAG)

$(STATIC_SERTARGET_FILE):$(SERVEROBJ)
	ar cr $(STATIC_SERTARGET_FILE) $(SERVEROBJ)

$(CLITEST_TARGET_FILE):$(CLITESTOBJ)
	$(CC) -o $(CLITEST_TARGET_FILE) $(CLITESTOBJ) -O3 $(CLITEST_LDFLAG)

$(SERTEST_TARGET_FILE):$(SERTESTOBJ)
	$(CC) -o $(SERTEST_TARGET_FILE) $(SERTESTOBJ) -O3 $(SERTEST_LDFLAG)

clean:
	rm -rf $(THIRD_LIBRARY) $(STATIC_CLITARGET_FILE) $(SHARED_CLITARGET_FILE) $(CLIENTOBJ) $(CLITEST_TARGET_FILE)  $(STATIC_SERTARGET_FILE) $(SHARED_SERTARGET_FILE) $(SERVEROBJ) $(SERTEST_TARGET_FILE) $(SERTESTOBJ) $(CLITESTOBJ)
	cd ./state-threads-master/ && make clean


GCC		= g++
MAKE		= make
OPTIMIZE	= -O2 -DSUPPORT_LH7 -DMKSTEMP
PKG-CONFIG	= $(pkg-config --cflags glib-2.0)
INCLUDE		= -I/usr/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/opt/project-remoteVideo/shared -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include
CFLAGS		= -std=c++11 -Winline -pipe -g $(INCLUDE)
LDLIBS		= -lpthread -lSocket -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lUseful

all: VideoClient.so videoServer.so

tests :  ServerTest ClientTest

videoServer.so : videoServer/videoServer.cpp videoServer/videoServer.h ConfigReader.o
	$(GCC) $(CFLAGS) -fPIC -shared videoServer/videoServer.cpp ConfigReader.o -o libVideoServer.so.1 $(LDLIBS)
	mv libVideoServer.so.1 libs/

ConfigReader.o : shared/configReader.cpp shared/configReader.h shared/videoStreamer_global.h
	$(GCC) -c $(CFLAGS) -fPIC shared/configReader.cpp -o ConfigReader.o

ServerTest : test.cpp
	$(GCC) $(CFLAGS) test.cpp -o server $(LDLIBS) -lVideoServer

VideoClient.so : videoClient/videoClient.cpp videoClient/videoClient.h ConfigReader.o
	$(GCC) $(CFLAGS) -fPIC -shared videoClient/videoClient.cpp ConfigReader.o -o libVideoClient.so.1
	mv libVideoClient.so.1 libs/

ClientTest : videoClient/main.cpp videoClient/videoClient.cpp videoClient/videoClient.h
	$(GCC) $(CFLAGS) videoClient/main.cpp videoClient/videoClient.cpp -o client $(LDLIBS) -lVideoClient

install :
	scp videoServer/videoServer.h /usr/include/
	scp videoClient/videoClient.h /usr/include/
	scp shared/videoStreamer_global.h /usr/include/videoStreamer_global.h
	scp libs/libVideoServer.so.1 /usr/local/lib/libVideoServer.so
	scp libs/libVideoClient.so.1 /usr/local/lib/libVideoClient.so
	chmod +x /usr/local/lib/libVideoServer.so
	chmod +x /usr/local/lib/libVideoClient.so
	ldconfig

uninstall :
	rm /usr/include/videoServer.h
	rm /usr/include/videoClient.h
	rm /usr/include/videoStreamer_global.h
	rm /usr/local/lib/libVideoServer.so
	rm /usr/local/lib/libVideoClient.so

clean :
	rm *.o
	rm libs/libVideoClient.so.1
	rm libs/libVideoServer.so.1

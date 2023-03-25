
#INCLUDES
INC = 1

# pck-config options
# RTSP_SERVER
GST_RTSP_SRV_CONFIG = `pkg-config --cflags --libs gstreamer-rtsp-server-1.0 gstreamer-1.0`
GST_CONFIG = `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-net-1.0`
MYSQLCONFIG = `pkg-config --cflags --libs mariadb`

all: $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB)ringbuffer joiner


# internal libs
GSTHELPERESINCLUDE = gstreamHelpers
GSTHELPERLIB = $(GSTHELPERESINCLUDE)/libgstreamHelpers.a
MYPLUGINSLIB = $(GSTHELPERESINCLUDE)/libmyplugins.a
HELPERBINS = $(GSTHELPERESINCLUDE)/helperBins
NMEALIB = $(GSTHELPERESINCLUDE)/libnematode.a

$(GSTHELPERLIB): $(wildcard $(GSTHELPERESINCLUDE)/*.cpp) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	make -C $(GSTHELPERESINCLUDE) helperlib

$(MYPLUGINSLIB):
	make -C $(GSTHELPERESINCLUDE) myplugins 

$(NMEALIB):
	make -C $(GSTHELPERESINCLUDE) nmealib 


MDNS_CPP_DIR = mdns_cpp
MDNS_BUILD_DIR = $(MDNS_CPP_DIR)/build
MDNS_CPP_INC = $(MDNS_CPP_DIR)/include
MDNS_CPP_LIB = $(MDNS_CPP_DIR)/build/lib/libmdns_cpp.a

$(MDNS_CPP_LIB):
	- mkdir $(MDNS_BUILD_DIR)
	cd $(MDNS_BUILD_DIR) && cmake .. && cd .. && cd ..
	make -C $(MDNS_BUILD_DIR)


ringbuffer: ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	g++ -g -o $@ ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(GST_CONFIG) $(MYSQLCONFIG) -I $(MDNS_CPP_INC) 
	sudo setcap cap_net_admin=eip ./ringbuffer

joiner: joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h) $(wildcard ./*.h)
	g++ -g -o $@ joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(GST_CONFIG) $(MYSQLCONFIG) -I $(MDNS_CPP_INC) 


# preceeding - means 'let it fail'
clean:
	-rm ./dots/*
#	-rm ./vids/out.mp4

package_all: package_server

package_server:
	- mkdir .debpkg-server/usr/
	- mkdir .debpkg-server/usr/bin
	cp ringbuffer .debpkg-server/usr/bin/
	cp joiner .debpkg-server/usr/bin/
	fakeroot dpkg-deb --build .debpkg-server
	mv .debpkg-server.deb dashcam-server.deb


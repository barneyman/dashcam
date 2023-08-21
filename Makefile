
#INCLUDES
INC = 1

ARCH=amd64
DEBARCH=$(ARCH)

CPPFLAGS = -O0 -Wformat -ggdb3
PRODFLAGS = -O3 -g0


# pck-config options
# RTSP_SERVER
# GST_RTSP_SRV_CONFIG = `pkg-config --cflags --libs gstreamer-rtsp-server-1.0 gstreamer-1.0`

GST_CONFIG = `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-net-1.0`
# when installed via apt
#MYSQLCONFIG = `pkg-config --cflags --libs mariadb`
# when built

MYSQLCONFIG = `pkg-config --cflags --libs libmariadb`
GPSD_CONFIG = `pkg-config --cflags --libs libgps`

all: $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) caps joiner test_sql
github: 
github: CPPFLAGS=$(PRODFLAGS)
github: $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) ringbuffer joiner package_apps

# internal libs
GSTHELPERESINCLUDE = gstreamHelpers
GSTHELPERLIB = $(GSTHELPERESINCLUDE)/libgstreamHelpers.a
MYPLUGINSLIB = $(GSTHELPERESINCLUDE)/libmyplugins.a
HELPERBINS = $(GSTHELPERESINCLUDE)/helperBins
NMEALIB = $(GSTHELPERESINCLUDE)/libnematode.a

$(GSTHELPERLIB): $(wildcard $(GSTHELPERESINCLUDE)/*.cpp) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	make CPPFLAGS="$(CPPFLAGS)" -C $(GSTHELPERESINCLUDE) helperlib

$(MYPLUGINSLIB):
	make CPPFLAGS="$(CPPFLAGS)" -C $(GSTHELPERESINCLUDE) myplugins 

$(NMEALIB):
	make CPPFLAGS="$(CPPFLAGS)"-C $(GSTHELPERESINCLUDE) nmealib 


MDNS_CPP_DIR = mdns_cpp
MDNS_BUILD_DIR = $(MDNS_CPP_DIR)/build
MDNS_CPP_INC = $(MDNS_CPP_DIR)/include
MDNS_CPP_LIB = $(MDNS_CPP_DIR)/build/lib/libmdns_cpp.a

$(MDNS_CPP_LIB):
	- mkdir $(MDNS_BUILD_DIR)
	cd $(MDNS_BUILD_DIR) && cmake .. && cd .. && cd ..
	make -C $(MDNS_BUILD_DIR)


ringbuffer: ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(GST_CONFIG) $(MYSQLCONFIG) $(GPSD_CONFIG) -I $(MDNS_CPP_INC) 

joiner: joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h) $(wildcard ./*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(NMEALIB) $(MDNS_CPP_LIB) $(GST_CONFIG) $(MYSQLCONFIG) -I $(MDNS_CPP_INC) 

test_sql: test_sql.cpp $(wildcard ./*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_sql.cpp $(MYSQLCONFIG) $(GST_CONFIG)

test_nobins: test_nobins.cpp $(wildcard ./*.h) $(GSTHELPERLIB) $(wildcard $(HELPERBINS)/*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_nobins.cpp $(GST_CONFIG) $(GSTHELPERLIB) $(MYPLUGINSLIB)

test_gpsd: test_gpsd.cpp 
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_gpsd.cpp $(GPSD_CONFIG)

caps: ringbuffer
	sudo setcap cap_net_admin=eip ./ringbuffer-$(ARCH)

# preceeding - means 'let it fail'
clean:
	-rm ./dots/*
#	-rm ./vids/out.mp4

package_all: package_apps

package_apps:
	- mkdir .debpkg-server/usr/
	- mkdir .debpkg-server/usr/bin
	sed -i 's/Architecture:.*/Architecture: $(ARCH)/' .debpkg-server/DEBIAN/control
	sed -i 's/Package:.*/Package: dashcam-server-$(ARCH)/' .debpkg-server/DEBIAN/control
	cp ringbuffer-$(ARCH) .debpkg-server/usr/bin/
	cp joiner-$(ARCH) .debpkg-server/usr/bin/
	fakeroot dpkg-deb --build .debpkg-server
	mv .debpkg-server.deb ./ringbuffer-$(ARCH).deb


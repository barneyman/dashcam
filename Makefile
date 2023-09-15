
#INCLUDES
INC = 1

ARCH=amd64
DEBARCH=$(ARCH)

CPPFLAGS = -O0 -Wformat -ggdb3
PRODFLAGS = -O3 -g0
LEAKFLAGS = -O0 -Wformat -ggdb3 -fsanitize=address -fno-omit-frame-pointer

# pck-config options
# RTSP_SERVER
# GST_RTSP_SRV_CONFIG = `pkg-config --cflags --libs gstreamer-rtsp-server-1.0 gstreamer-1.0`

GST_CONFIG = `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-net-1.0`
# when installed via apt
#MYSQLCONFIG = `pkg-config --cflags --libs mariadb`
# when built

MYSQLCONFIG = `pkg-config --cflags --libs libmariadb`
GPSD_CONFIG = `pkg-config --cflags --libs libgps`

AVAHI_CONFIG = `pkg-config --cflags --libs avahi-glib avahi-client`

all: $(GSTHELPERLIB) $(MYPLUGINSLIB) caps joiner test_sql

github: 
github: CPPFLAGS=$(PRODFLAGS)
github: $(GSTHELPERLIB) $(MYPLUGINSLIB) ringbuffer joiner package_apps

leaks:
leaks: CPPFLAGS=$(LEAKFLAGS)
leaks: $(GSTHELPERLIB) $(MYPLUGINSLIB) ringbuffer joiner 

# internal libs
GSTHELPERESINCLUDE = gstreamHelpers
GSTHELPERLIB = $(GSTHELPERESINCLUDE)/libgstreamHelpers.a
MYPLUGINSLIB = $(GSTHELPERESINCLUDE)/libmyplugins.a
HELPERBINS = $(GSTHELPERESINCLUDE)/helperBins

$(GSTHELPERLIB): $(wildcard $(GSTHELPERESINCLUDE)/*.cpp) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	make CPPFLAGS="$(CPPFLAGS)" -C $(GSTHELPERESINCLUDE) helperlib

$(MYPLUGINSLIB):
	make CPPFLAGS="$(CPPFLAGS)" -C $(GSTHELPERESINCLUDE) myplugins 

ringbuffer: ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(AVAHI_CONFIG) $(GST_CONFIG) $(MYSQLCONFIG) $(GPSD_CONFIG) 

joiner: joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h) $(wildcard ./*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(GST_CONFIG) $(MYSQLCONFIG) 

test_sql: test_sql.cpp $(wildcard ./*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_sql.cpp $(MYSQLCONFIG) $(GST_CONFIG)

test_nobins: test_nobins.cpp $(wildcard ./*.h) $(GSTHELPERLIB) $(wildcard $(HELPERBINS)/*.h)
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_nobins.cpp $(GST_CONFIG) $(GSTHELPERLIB) $(MYPLUGINSLIB)

test_gpsd: test_gpsd.cpp 
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_gpsd.cpp $(GPSD_CONFIG)

test_composite: test_composite.cpp
	g++ $(CPPFLAGS) -o $@-$(ARCH) test_composite.cpp $(GST_CONFIG) $(GSTHELPERLIB) $(MYPLUGINSLIB)

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


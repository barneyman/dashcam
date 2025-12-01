
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

all: $(GSTHELPERLIB) $(MYPLUGINSLIB) $(HIREDISSHAREDLIB) caps ringbuffer joiner test_sql

github: 
github: CPPFLAGS=$(PRODFLAGS)
github: $(GSTHELPERLIB) $(MYPLUGINSLIB) $(HIREDISSHAREDLIB) ringbuffer joiner package_apps

leaks:
leaks: CPPFLAGS=$(LEAKFLAGS)
leaks: $(GSTHELPERLIB) $(MYPLUGINSLIB) ringbuffer joiner 


# hiredis lib
HIREDIS = hiredis
HIREDISSHAREDLIB =$(HIREDIS)/build/libhiredis.so
$(HIREDISSHAREDLIB):$(wildcard $(HIREDIS)/*.c) $(wildcard $(HIREDIS)/*.h)
	mkdir -p $(HIREDIS)/build
	cd $(HIREDIS)/build && cmake .. && make

hiredis: $(HIREDISSHAREDLIB)


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
	g++ $(CPPFLAGS) -o $@ ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(AVAHI_CONFIG) $(GST_CONFIG) $(MYSQLCONFIG) $(GPSD_CONFIG) 

joiner: joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(wildcard $(HELPERBINS)/*.h) $(wildcard $(GSTHELPERESINCLUDE)/*.h) $(wildcard ./*.h)
	g++ $(CPPFLAGS) -o $@ joiner.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(GST_CONFIG) $(MYSQLCONFIG) 

test_sql: test_sql.cpp $(wildcard ./*.h)
	g++ $(CPPFLAGS) -o $@ test_sql.cpp $(MYSQLCONFIG) $(GST_CONFIG)

test_nobins: test_nobins.cpp $(wildcard ./*.h) $(GSTHELPERLIB) $(wildcard $(HELPERBINS)/*.h)
	g++ $(CPPFLAGS) -o $@ test_nobins.cpp $(GST_CONFIG) $(GSTHELPERLIB) $(MYPLUGINSLIB)

test_gpsd: test_gpsd.cpp 
	g++ $(CPPFLAGS) -o $@ test_gpsd.cpp $(GPSD_CONFIG)

test_composite: test_composite.cpp
	g++ $(CPPFLAGS) -o $@ test_composite.cpp $(GST_CONFIG) $(GSTHELPERLIB) $(MYPLUGINSLIB)

caps: ringbuffer
	sudo setcap cap_net_admin=eip ./ringbuffer

# preceeding - means 'let it fail'
clean:
	-rm ./dots/*
#	-rm ./vids/out.mp4

package_all: all package_apps

package_apps: ringbuffer joiner $(HIREDISSHAREDLIB)
	- mkdir -p .debpkg-server/usr/
	- mkdir -p .debpkg-server/usr/bin
	- mkdir -p .debpkg-server/usr/lib
	sed -i 's/Architecture:.*/Architecture: $(ARCH)/' .debpkg-server/DEBIAN/control
	sed -i 's/Package:.*/Package: dashcam-server-$(ARCH)/' .debpkg-server/DEBIAN/control
	cp ringbuffer .debpkg-server/usr/bin/
	cp joiner .debpkg-server/usr/bin/
	cp $(HIREDISSHAREDLIB)* .debpkg-server/usr/lib/
	fakeroot dpkg-deb --build .debpkg-server
	mv .debpkg-server.deb ./ringbuffer-$(ARCH).deb


# intended for local development
docker_all: docker_sql docker_rtsp

docker_sql: 
#  exploit 'each line runs in its own sh` to not need pushd and popd
	cd buildx/other/mariadb && docker build --build-arg BUILDFROM=mariadb:11.4 --build-arg ROOTPWD=password -f Dockerfile --tag debug/dashcam-mariadb:latest .

docker_sql_run: docker_sql
	docker compose -f buildx/other/mariadb/compose.yml up -V

docker_rtsp:
	cd buildx/other/rtsp-simple-server && docker build -f Dockerfile --tag debug/dashcam-rtsp:latest .

docker_rtsp_run: docker_rtsp
	docker compose -f buildx/other/rtsp-simple-server/compose.yml up


#INCLUDES
INC = 1

# pck-config options
# RTSP_SERVER
GST_RTSP_SRV_CONFIG = `pkg-config --cflags --libs gstreamer-rtsp-server-1.0 gstreamer-1.0`
GST_CONFIG = `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-net-1.0`
MYSQLCONFIG = `pkg-config --cflags --libs mariadb`


# internal libs
GSTHELPERESINCLUDE = gstreamHelpers
GSTHELPERLIB = $(GSTHELPERESINCLUDE)/libgstreamHelpers.a
MYPLUGINSLIB = $(GSTHELPERESINCLUDE)/libmyplugins.a
HELPERBINS = $(GSTHELPERESINCLUDE)/helperBins

$(GSTHELPERLIB): $(wildcard $(GSTHELPERESINCLUDE)/*.cpp) $(wildcard $(GSTHELPERESINCLUDE)/*.h)
	make -C $(GSTHELPERESINCLUDE) helperlib

$(MYPLUGINSLIB):
	make -C $(GSTHELPERESINCLUDE) myplugins 

rtsp-server-simple: rtsp-server-simple.cpp
	g++ -g -o $@ $(GST_RTSP_SRV_CONFIG) rtsp-server-simple.cpp


ringbuffer: ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(wildcard $(HELPERBINS)/*.h)
	g++ -g -o $@ ringbuffer.cpp $(GSTHELPERLIB) $(MYPLUGINSLIB) $(GST_CONFIG) $(MYSQLCONFIG)

# preceeding - means 'let it fail'
clean:
	-rm ./dots/*
	-rm ./out.mp4
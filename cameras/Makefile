CXX = g++
CCC = gcc
CPPFLAGS = -g3 -O0
PRODFLAGS = -O3 -g0

GSTCONFIG = `pkg-config --cflags --libs gstreamer-1.0`
GSTGESCONFIG = `pkg-config --cflags --libs gstreamer-1.0 gst-editing-services-1.0`
GSTGETBASECONFIG = `pkg-config --cflags --libs gstreamer-1.0 gst-editing-services-1.0 gstreamer-base-1.0`

GSTRTSPCONFIG = `pkg-config --cflags --libs gstreamer-rtsp-server-1.0 gstreamer-1.0`



CXXFLAGS = -Wno-psabi  
LDFLAGS = 



GSTHELPERESINCLUDE = gstreamHelpers
GSTHELPERLIB = $(GSTHELPERESINCLUDE)/gstreamHelpers.a

RTSPSRC = test-launch.c 
RTSPOBJ = $(RTSPSRC:.cc=.o)
RTSPSERVER = rtsp-srv
RTSPSERVERCXXFLAGS = $(CXXFLAGS) $(GSTRTSPCONFIG)



rtsp: $(RTSPSERVER)


# apps
$(RTSPSERVER): $(RTSPOBJ) 
	$(CCC) -c mdns_advertise.c
	$(CXX) $(LDFLAGS) -o $@ $(RTSPOBJ) mdns_advertise.o $(CPPFLAGS) $(RTSPSERVERCXXFLAGS) -I $(GSTHELPERESINCLUDE)



clean:
	rm -rf *.o $(EXEC) $(CLIENTEXEC) $(FLVEXEC) $(SERVEREXEC) 

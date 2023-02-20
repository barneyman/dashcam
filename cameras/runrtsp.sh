#!/bin/sh
#./rtsp-srv --gst-debug=3 '( libcamerasrc ! video/x-raw,format=YUY2, width=640, height=480, framerate=30/1 ! videoconvert ! videoscale ! v4l2h264enc ! ! video/x-h264,level=(string)3 ! h264parse ! rtph264pay name=pay0 pt=96 )'
#./rtsp-srv --gst-debug=3 '( libcamerasrc ! video/x-raw,format=YUY2,width=640, height=480 ! videoscale ! videoconvert ! v4l2h264enc ! video/x-h264,level=(string)3 ! h264parse ! rtph264pay name=pay0 pt=96 )'
#./rtsp-srv --gst-debug=3 '( libcamerasrc ! video/x-raw,width=1280,height=720,format=NV12 ! v4l2convert ! v4l2h264enc extra-controls="controls,repeat_sequence_header=1" ! video/x-h264,level=(string)4 ! h264parse ! queue2 ! rtph264pay name=pay0 pt=96 )'

# right at the margins of what a pi zero can do
./rtsp-srv --gst-debug=3 '( libcamerasrc ! video/x-raw,width=1280,height=720,format=NV12 ! queue ! v4l2convert ! queue2 ! v4l2h264enc extra-controls="controls,repeat_sequence_header=1" ! video/x-h264,level=(string)3.1 ! queue ! h264parse ! rtph264pay name=pay0 pt=96 )'
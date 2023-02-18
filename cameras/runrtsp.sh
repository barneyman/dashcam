#!/bin/sh
#./rtsp-srv --gst-debug=3 '( libcamerasrc ! video/x-raw,format=YUY2, width=640, height=480, framerate=30/1 ! videoconvert ! videoscale ! v4l2h264enc ! ! video/x-h264,level=(string)3 ! h264parse ! rtph264pay name=pay0 pt=96 )'
./rtsp-srv --gst-debug=3 '( libcamerasrc ! video/x-raw,format=YUY2,width=640, height=480 ! videoscale ! videoconvert ! v4l2h264enc ! video/x-h264,level=(string)3 ! h264parse ! rtph264pay name=pay0 pt=96 )'

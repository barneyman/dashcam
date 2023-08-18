#include "gstreamHelpers/gstreamPipeline.h"
#include "gstreamHelpers/helperBins/remoteSourceBins.h"

int main()
{

    gstreamPipeline pipeline("testPipe");

#define _USE_RTSP_BIN
#ifdef _USE_RTSP_BIN
    //rtspSourceBin rtspbin(&pipeline,"rtsp://vpnhack:8554/cam");
    std::vector<std::string> srv={"rtsp://vpnhack:8554/cam"};
    multiRemoteSourceBin<rtspSourceBin> rtspbin(&pipeline,srv,"video/x-h264,stream-format=(string)avc,alignment=(string)au");
#else
    // rtsp
    pipeline.AddPlugin("rtspsrc","rtspsrc");
    pipeline.AddPlugin("h264parse","h264parse");
    pipeline.AddPlugin("rtph264depay","rtph264depay");

    // and config
    g_object_set (pipeline.FindNamedPlugin("rtspsrc"), 
        "location","rtsp://vpnhack:8554/cam",
        NULL);

#endif

    pipeline.AddPlugin("splitmuxsink");

    g_object_set (pipeline.FindNamedPlugin("splitmuxsink"), 
        "location","/vids/nobins_%05d.mp4",
        NULL);

#ifdef _USE_RTSP_BIN
    gst_element_link_many(    pipeline.FindNamedPlugin(rtspbin),
                    pipeline.FindNamedPlugin("splitmuxsink"),
                    NULL);
#else
    // rtsp connects late
    pipeline.ConnectPipelineLate(    pipeline.FindNamedPlugin("rtspsrc"),
                    pipeline.FindNamedPlugin("rtph264depay"));

    // sink it
    // pipeline.AddPlugin("fakeseink");
    // g_object_set (pipeline.FindNamedPlugin("fbdevsink"), 
    //     "dev","/dev/fb0",
    //     NULL);

    // then all the others ..
    gst_element_link_many(
            //pipeline.FindNamedPlugin("rtspsrc"),
            pipeline.FindNamedPlugin("rtph264depay"),
            pipeline.FindNamedPlugin("h264parse"),
            pipeline.FindNamedPlugin("splitmuxsink"),
            NULL
        );
#endif

    pipeline.Run(15);

    return 0;
}
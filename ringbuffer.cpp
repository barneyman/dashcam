#include "gstreamHelpers/helperBins/remoteSourceBins.h"
#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"

int main()
{
    gstreamPipeline thePipeline("mainPipeline");
    
    const char *location="rtsp://vpnhack:8554/cam";
    const char *destination="/workspaces/dashcam//out.mp4";

    thePipeline.AddPlugin("rtspsrc","rtspsrc");
    thePipeline.AddPlugin("rtph264depay","depay2");
    thePipeline.AddPlugin("rtpjitterbuffer","antijitter");
    thePipeline.AddPlugin("h264parse","parser2");

    // and config
    g_object_set (thePipeline.FindNamedPlugin("rtspsrc"), 
        "location",location,
        NULL);

    // rtsp connects late
    thePipeline.ConnectPipelineLate(    thePipeline.FindNamedPlugin("rtspsrc"),
                    thePipeline.FindNamedPlugin("antijitter"));

    // then all the others ..
    gst_element_link_many(
            thePipeline.FindNamedPlugin("antijitter"),
            thePipeline.FindNamedPlugin("depay2"),
            thePipeline.FindNamedPlugin("parser2"),
            NULL
        );

    thePipeline.AddPlugin("mp4mux","muxer");

    thePipeline.AddPlugin("filesink","finalsink");
    g_object_set (thePipeline.FindNamedPlugin("finalsink"), 
        "location", destination, NULL);

    gst_element_link_many( thePipeline.FindNamedPlugin("parser2"),
        thePipeline.FindNamedPlugin("muxer"),
        thePipeline.FindNamedPlugin("finalsink"),
        NULL
        );


    thePipeline.DumpGraph("prerun");

    thePipeline.Run(20);

    thePipeline.DumpGraph("postrun");

    return 1;





}
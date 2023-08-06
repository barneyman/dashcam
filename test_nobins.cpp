#include "gstreamHelpers/gstreamPipeline.h"

int main()
{

    gstreamPipeline pipeline("testPipe");

    // rtsp
    pipeline.AddPlugin("rtspsrc","rtspsrc");
    pipeline.AddPlugin("rtph264depay","depay2");
    pipeline.AddPlugin("identity","antijitter");
    pipeline.AddPlugin("h264parse","parser2");

    // and config
    g_object_set (pipeline.FindNamedPlugin("rtspsrc"), 
        "location","rtsp://vpnhack:8554/cam",
        NULL);

    // rtsp connects late
    pipeline.ConnectPipelineLate(    pipeline.FindNamedPlugin("rtspsrc"),
                    pipeline.FindNamedPlugin("antijitter"));

    // sink it
    // pipeline.AddPlugin("fakeseink");
    // g_object_set (pipeline.FindNamedPlugin("fbdevsink"), 
    //     "dev","/dev/fb0",
    //     NULL);

    // then all the others ..
    gst_element_link_many(
            pipeline.FindNamedPlugin("antijitter"),
            pipeline.FindNamedPlugin("depay2"),
            pipeline.FindNamedPlugin("parser2"),
            pipeline.FindNamedPlugin("fakeseink"),
            NULL
        );

    pipeline.Run(120);

    return 0;
}
#include "gstreamHelpers/gstreamPipeline.h"
#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myDemuxBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"

int main()
{

    gstreamPipeline pipeline("testPipe");

    gstVideoMixerBin mixer(&pipeline);

    gstMP4DemuxDecodeBin video1(&pipeline,"/vids/grabs/79fb7d3f-4ecb-11ee-8ce5-0242ac120002.mp4");    
    gstMP4DemuxDecodeBin video2(&pipeline,"/vids/grabs/79fb7d3f-4ecb-11ee-8ce5-0242ac120002.mp4",GST_CLOCK_TIME_NONE,GST_CLOCK_TIME_NONE,true,NULL,"steve");    

    gstH264encoderBin encoder(&pipeline);

    gstMP4OutBin output(&pipeline,"/vids/all.mp4");

    pipeline.DumpGraph("pre-composite");
    if(!pipeline.ConnectPipeline(video2,mixer) 
            && !pipeline.ConnectPipeline(video1,mixer)
            && !pipeline.ConnectPipeline(mixer,output,encoder)
            )
    {
        pipeline.DumpGraph("composite");
        pipeline.Run();
    }
    else
    {
        pipeline.DumpGraph("composite-failed");

    }
}
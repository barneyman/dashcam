#include "gstreamHelpers/helperBins/remoteSourceBins.h"
#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"

// sql
#include <mysql.h>


class ringBufferPipeline : public gstreamPipeline
{

public:
    ringBufferPipeline(const char*src, const char *out):
        gstreamPipeline("ringBufferPipeline"),
        m_sourceBin(this,src,"source"),
        m_h264Caps(this,"video/x-h264,stream-format=(string)avc,alignment=(string)au"),
        m_sinkBin(this,10,out)
    {
        
        ConnectPipeline("source","splitMuxOutBin","capsFilterBin");

    }

    void SplitMuxOpenedSplit(GstClockTime, const char*newFile)
    {

    }

    void SplitMuxClosedSplit(GstClockTime, const char*newFile)
    {
        
    }


    // there's a barely documented message from splitmuxsink when it splits and closes
    // https://stackoverflow.com/questions/65023233/gstreamer-splitmuxsink-callback-when-a-new-file-is-created
    // splitmuxsink-fragment-opened & splitmuxsink-fragment-closed
    virtual void elementMessageHandler(GstMessage*msg)
    {

        if(gst_message_has_name(msg,"splitmuxsink-fragment-opened"))
        {
            /*
            "location", G_TYPE_STRING, location,
            "running-time", GST_TYPE_CLOCK_TIME, running_time,                
            */
            const GstStructure *str=gst_message_get_structure(msg);
            const char* location=gst_structure_get_string(str,"location");
            
            GstClockTime timestamp;
            gst_structure_get_clock_time(str,"running-time",&timestamp);

            SplitMuxOpenedSplit(timestamp, location);

        }
        else if(gst_message_has_name(msg,"splitmuxsink-fragment-closed"))
        {
            /*
            "location", G_TYPE_STRING, location,
            "running-time", GST_TYPE_CLOCK_TIME, running_time,                
            */
            const GstStructure *str=gst_message_get_structure(msg);
            const char* location=gst_structure_get_string(str,"location");
            
            GstClockTime timestamp;
            gst_structure_get_clock_time(str,"running-time",&timestamp);

            SplitMuxClosedSplit(timestamp, location);

        }        
        
        // abd call down
        gstreamPipeline::elementMessageHandler(msg);
    }




protected:

    rtspSourceBin m_sourceBin;
    gstCapsFilterSimple m_h264Caps;
    gstSplitMuxOutBin m_sinkBin;

};


int main()
{
    const char *location="rtsp://vpnhack:8554/cam";
    const char *destination="/workspaces/dashcam/out_%05d.mp4";

    ringBufferPipeline thePipeline(location,destination);

    thePipeline.DumpGraph("prerun");

    thePipeline.Run(60);

    thePipeline.DumpGraph("postrun");

    return 1;





}
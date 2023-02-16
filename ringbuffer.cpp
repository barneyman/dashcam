#include "gstreamHelpers/helperBins/remoteSourceBins.h"
#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"
#include "gstreamHelpers/helperBins/myDemuxBins.h"

// sql
#include <mysql.h>


class ringBufferPipeline : public gstreamPipeline
{

public:
    ringBufferPipeline(const char*src, const char *out):
        gstreamPipeline("ringBufferPipeline"),
        m_sourceBin(this,src,"source"),
        m_h264Caps(this,"video/x-h264,stream-format=(string)avc,alignment=(string)au"),
        m_sinkBin(this,60,out),
        m_progress(this)
    {
        //ConnectPipeline(m_sourceBin,m_sinkBin,m_h264Caps);
        gst_element_link_many(FindNamedPlugin(m_sourceBin),
            FindNamedPlugin(m_h264Caps),
            FindNamedPlugin(m_progress),
            FindNamedPlugin(m_sinkBin),
            NULL);
    }

    virtual void SplitMuxOpenedSplit(GstClockTime, const char*newFile)
    {

    }

    virtual void SplitMuxClosedSplit(GstClockTime, const char*newFile)
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
    gstFrameBufferProgress m_progress;

};


class joinVidsPipeline : public gstreamPipeline
{
public:
    joinVidsPipeline(std::vector<std::string> &files, const char*destination):
        gstreamPipeline("joinVidsPipeline"),
        m_multisrc(this,files),
        m_out(this,destination)

    {
        ConnectPipeline(m_multisrc,m_out);
    }

protected:

        gstMP4DemuxDecodeSparseBin m_multisrc;
        gstMP4OutBin m_out;

};




#define SPLIT_SINK


int main()
{
    const char *location="rtsp://vpnhack:8554/cam";
    const char *destination="/workspaces/dashcam/out_%05d.mp4";

    gstreamPipeline thePipeline("mainPipeline");


#ifdef SPLIT_SINK

    ringBufferPipeline ringPipeline(location,destination);
    ringPipeline.Run(360);


#else


    std::vector<std::string> files;
    
    files.push_back("/workspaces/dashcam/out_00000.mp4");
    files.push_back("/workspaces/dashcam/out_00001.mp4");
    files.push_back("/workspaces/dashcam/out_00002.mp4");
    files.push_back("/workspaces/dashcam/out_00003.mp4");
    files.push_back("/workspaces/dashcam/out_00004.mp4");
    files.push_back("/workspaces/dashcam/out_00005.mp4");

    // gstMP4DemuxDecodeSparseBin multiSrc(&thePipeline,files);
    // gstMP4OutBin out(&thePipeline,"/workspaces/dashcam/combined.mp4");
    // thePipeline.ConnectPipeline(multiSrc,out);

    // thePipeline.Run();

    joinVidsPipeline joiner(files,"/workspaces/dashcam/combined.mp4");
    joiner.Run();


#endif

    return 1;





}
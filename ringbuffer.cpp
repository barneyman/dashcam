#include "gstreamHelpers/helperBins/remoteSourceBins.h"
#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"
#include "gstreamHelpers/helperBins/myDemuxBins.h"
#include "metaBins.h"

// sql
#include <mysql.h>
// mdns

#include <thread>
#include <chrono>

#include "mdns_cpp/mdns.hpp"
#include "mdns_cpp/macros.hpp"
#include "mdns_cpp/utils.hpp"




class ringBufferPipeline : public gstreamPipeline
{

public:
    ringBufferPipeline(const char*src, const char *out):
        gstreamPipeline("ringBufferPipeline"),
        m_sourceBins(NULL),
        m_sinkBin(this,60,out),
        m_browserDone(false),
        m_fatal(false),
        m_nmea(this),
        m_browser(staticBrowse,this)

    {
        // use NTP clock
        UseNTPv4Clock();

        while(!m_browserDone)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // use real time
        //setRealtimeClock();

        // mark up the records
        std::vector<std::string> urls;
        for(auto each=arecords.begin();each!=arecords.end();each++)
        {
            // rtsp://vpnhack:8554/cam
            std::string url("rtsp://");
            url+=*each;
            url+=":8554/cam";
            urls.push_back(url);
        }

        m_sourceBins=new multiRemoteSourceBin<rtspSourceBin>(this,urls,"video/x-h264,stream-format=(string)avc,alignment=(string)au");

        m_fatal=buildPipeline();
    }

    ~ringBufferPipeline()
    {
        delete m_sourceBins;
        m_browser.join();
    }

    static void staticBrowse(ringBufferPipeline *owner)
    {
        owner->Browse();
    }

    void Browse()
    {
        mdns_cpp::mDNS mdns;
        mdns.executeQuery("_dashcam._tcp.local.",arecords);
        m_browserDone=true;
    }    

    bool buildPipeline()
    {
        // //ConnectPipeline(m_nmea,m_sinkBin);
        GstCaps *subtitlecaps=gst_caps_from_string("text/any");
        //gst_element_link_filtered(FindNamedPlugin(m_nmea),FindNamedPlugin(m_sinkBin),subtitlecaps);
        bool success=gst_element_link_pads(FindNamedPlugin(m_nmea),NULL,FindNamedPlugin(m_sinkBin),"subtitle_%u");


        // //ConnectPipeline(*m_sourceBins,m_sinkBin);
        GstCaps *videocaps=gst_caps_from_string("video/any");
//        success=gst_element_link_filtered(FindNamedPlugin(*m_sourceBins),FindNamedPlugin(m_sinkBin),videocaps);
        success=gst_element_link(FindNamedPlugin(*m_sourceBins),FindNamedPlugin(m_sinkBin));


        gst_caps_unref(videocaps);
        gst_caps_unref(subtitlecaps);
        
        // gst_element_link(FindNamedPlugin(*m_sourceBins),FindNamedPlugin(m_sinkBin));
        // gst_element_link(FindNamedPlugin(m_nmea),FindNamedPlugin(m_sinkBin));

        return true;
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

    bool m_browserDone, m_fatal;
    std::vector<std::string> arecords;
    
    multiRemoteSourceBin<rtspSourceBin> *m_sourceBins;
    gstSplitMuxOutBin m_sinkBin;
    std::thread m_browser;
    gstNmeaToSubs m_nmea;
};

#define _USE_META
class joinVidsPipeline : public gstreamPipeline
{
public:
    joinVidsPipeline(std::vector<std::string> &files, const char*destination):
        gstreamPipeline("joinVidsPipeline"),
        m_multisrc(this,files),
#ifdef _USE_META        
        m_meta(this),
#endif        
        m_out(this,destination)

    {
#ifdef _USE_META        
        ConnectPipeline(m_multisrc,m_out,m_meta);
#else        
        ConnectPipeline(m_multisrc,m_out);
#endif        
    }

protected:

        gstMP4DemuxDecodeSparseBin m_multisrc;
        gstMP4OutBin m_out;
#ifdef _USE_META        
        gstMultiNmeaJsonToPangoRenderBin m_meta;
#endif
};




#define SPLIT_SINK

#include "gstreamHelpers/myplugins/gstnmeasource.h"

int main()
{
    const char *location="rtsp://vpnhack:8554/cam";
    const char *destination="/workspaces/dashcam/out_%05d.mp4";

    if(0)
    {
        gstreamPipeline thePipeline("mainPipeline");
        ringBufferPipeline ringPipeline(location,destination);
        ringPipeline.Run(30);
        return 1;
    }

    std::vector<std::string> files={
        "/workspaces/dashcam/out_00000.mp4"
    };
    
    //files.push_back("/workspaces/dashcam/out_00000.mp4");
    // files.push_back("/workspaces/dashcam/out_00001.mp4");
    // files.push_back("/workspaces/dashcam/out_00002.mp4");
    // files.push_back("/workspaces/dashcam/out_00003.mp4");
    // files.push_back("/workspaces/dashcam/out_00004.mp4");
    // files.push_back("/workspaces/dashcam/out_00005.mp4");

    // gstMP4DemuxDecodeSparseBin multiSrc(&thePipeline,files);
    // gstMP4OutBin out(&thePipeline,"/workspaces/dashcam/combined.mp4");
    // thePipeline.ConnectPipeline(multiSrc,out);

    // thePipeline.Run();

    joinVidsPipeline joiner(files,"/workspaces/dashcam/combined.mp4");
    joiner.Run();



    return 1;





}
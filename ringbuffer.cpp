#include "gstreamHelpers/helperBins/remoteSourceBins.h"
#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"
#include "gstreamHelpers/myplugins/gstnmeasource.h"
#include "metaBins.h"

// sql
#include <mysql.h>

#include <thread>
#include <chrono>

// mdns
#include "mdns_cpp/mdns.hpp"
#include "mdns_cpp/macros.hpp"
#include "mdns_cpp/utils.hpp"


#define _USE_NMEA

class ringBufferPipeline : public gstreamPipeline
{

public:
    ringBufferPipeline(const char *out):
        gstreamPipeline("ringBufferPipeline"),
        m_sourceBins(NULL),
        m_sinkBin(this,60,out),
        m_browserDone(false),
        m_fatal(false),
#ifdef _USE_NMEA        
        m_nmea(this),
#endif        
        m_browser(staticBrowse,this)

    {
        // use NTP clock
        //UseNTPv4Clock();

        while(!m_browserDone)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // use real time
        setRealtimeClock();

        // mark up the records
        std::vector<std::string> urls={"rtsp://rpizerocam:8554/cam"};
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
#ifdef _USE_NMEA
        ConnectPipeline(m_nmea,m_sinkBin);
#endif

//        GstCaps *subtitlecaps=gst_caps_from_string("text/any");
        //gst_element_link_filtered(FindNamedPlugin(m_nmea),FindNamedPlugin(m_sinkBin),subtitlecaps);
        //bool success=gst_element_link_pads(FindNamedPlugin(m_nmea),NULL,FindNamedPlugin(m_sinkBin),"subtitle_%u");


        ConnectPipeline(*m_sourceBins,m_sinkBin);
//        GstCaps *videocaps=gst_caps_from_string("video/any");
//        success=gst_element_link_filtered(FindNamedPlugin(*m_sourceBins),FindNamedPlugin(m_sinkBin),videocaps);
//        success=gst_element_link(FindNamedPlugin(*m_sourceBins),FindNamedPlugin(m_sinkBin));


//        gst_caps_unref(videocaps);
        //gst_caps_unref(subtitlecaps);
        
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
#ifdef _USE_NMEA    
    gstNmeaToSubs m_nmea;
#endif    
};





int main()
{
    char buffer[200];

    struct tm *info; 
    time_t now=time(NULL);
    info = gmtime(&now);

    strftime(buffer,sizeof(buffer)-1,"/vids/%F%H%MZ_%%05d.mp4",info);

    const char *destination=buffer;

    gstreamPipeline thePipeline("mainPipeline");
    ringBufferPipeline ringPipeline(destination);
    ringPipeline.Run(360);

}
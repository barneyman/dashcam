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

#ifdef _DEBUG_TIMESTAMPS
#include "gstreamHelpers/myplugins/gstptsnormalise.h"
#endif

#include "gstreamHelpers/helperBins/probeHelper.h"

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
        padProber(this),
        m_dataFlowing(false),
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
#ifdef _DEBUG_TIMESTAMPS
        ptsnormalise_registerRunTimePlugin();
        AddPlugin("ptsnormalise","ptsnormalise_subs");
        ConnectPipeline(m_nmea,m_sinkBin,"ptsnormalise_subs");
#else
        ConnectPipeline(m_nmea,m_sinkBin);
#endif        


#ifdef _DEBUG_TIMESTAMPS
        AddPlugin("ptsnormalise","ptsnormalise_video");
        ConnectPipeline(*m_sourceBins,m_sinkBin,"ptsnormalise_video");
#else
        ConnectPipeline(*m_sourceBins,m_sinkBin);
#endif
#endif

        // now set up the pad block - the RTSP takes some time to start playing
        // if we send subs before it the runtime gets bent and join fails with decreasing timestamps

        attachProbes("nmeasource",GST_PAD_PROBE_TYPE_BLOCK,true,&m_nmea);
        // TODO when handling multiple video streams this needs to block all until all streams are producing
        attachProbes(*m_sourceBins,GST_PAD_PROBE_TYPE_BUFFER);

        return true;
    }

    virtual GstPadProbeReturn blockProbe(GstPad * pad,GstPadProbeInfo * padinfo)
    {
        // }
         GST_INFO_OBJECT (m_pipeline, "Probetype %s\r",getProbeNames(padinfo).c_str());

        if(padinfo->type & GST_PAD_PROBE_TYPE_BUFFER)
        {
            if(m_dataFlowing)
            {
                return GST_PAD_PROBE_REMOVE;
            }
            GST_INFO_OBJECT (m_pipeline, "dropped");
            return GST_PAD_PROBE_DROP;
        }
        return GST_PAD_PROBE_PASS;
    }

    virtual void bufferProbe(GstPad * pad,GstPadProbeInfo * padinfo)
    {
        if(!m_dataFlowing)
        {
            GstBuffer *theBuffer=gst_pad_probe_info_get_buffer(padinfo);
            GstEvent *gap=gst_event_new_gap(0,theBuffer->pts);
            if(!gst_pad_push_event(pad,gap))
            {
                GST_ERROR_OBJECT (m_pipeline, "gst_pad_push_event gap failed");
            }
        }

        m_dataFlowing=true;
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
    volatile bool m_dataFlowing;
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
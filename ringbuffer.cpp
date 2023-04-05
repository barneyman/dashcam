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

#include "dashcam_sql.h"
#include "rb_tasks.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


#define _USE_NMEA
//#define _USE_MDNS
//#define _USE_PROBES
#define _USE_BARRIER

#ifdef _USE_BARRIER
#include "gstreamHelpers/myplugins/gstbarrier.h"
#endif

// sudo setcap cap_net_admin=eip ./ringbuffer

class ringBufferPipeline : 
    public gstreamPipeline, 
    public padProber
{

public:
    ringBufferPipeline(const char *outdir, unsigned sliceMins=15):
        gstreamPipeline("ringBufferPipeline"),
        m_sourceBins(NULL),
        m_browserDone(false),
        m_fatal(false),
#ifdef _USE_NMEA        
        m_nmea(this),
        padProber(this),
        m_dataFlowing(false),
#endif        
        m_browser(staticBrowse,this),
        m_sql("debian","dashcam","dashcam","dashcam")
    {
        // use NTP clock
        //UseNTPv4Clock();
#ifdef _USE_MDNS
        while(!m_browserDone)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
#endif        

        // use real time
        setRealtimeClock();

        char buffer[300];
        struct tm *info; 
        time_t now=time(NULL);
        info = gmtime(&now);

        m_outspec=outdir;

        strftime(buffer,sizeof(buffer)-1,"/%F%H%MZ_%%05d.mp4",info);
        m_outspec+=buffer;


        m_sinkBin=new gstSplitMuxOutBin(this,sliceMins*60,m_outspec.c_str());


        // mark up the records
        //std::vector<std::string> urls={"rtsp://rpizerocam:8554/cam"};
        std::vector<std::string> urls={"rtsp://vpnhack:8554/cam"};
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
        delete m_sinkBin;
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
#ifdef _USE_BARRIER
        barrier_registerRunTimePlugin();
        AddPlugin("barrier");
#endif

#ifdef _USE_NMEA
 #ifdef _DEBUG_TIMESTAMPS
        ptsnormalise_registerRunTimePlugin();
        AddPlugin("ptsnormalise","ptsnormalise_subs");
        ConnectPipeline(m_nmea,*m_sinkBin,"ptsnormalise_subs");
 #else
  #ifdef _USE_BARRIER
        bool linked=ConnectPipeline(m_nmea,*m_sinkBin,"barrier");
        //   bool linked=gst_element_link_many(
        //     FindNamedPlugin(m_nmea),
        //     FindNamedPlugin("barrier"),
        //     FindNamedPlugin(*m_sinkBin),
        //     NULL
        // );
#else
        ConnectPipeline(m_nmea,*m_sinkBin);
  #endif
 #endif        


 #ifdef _DEBUG_TIMESTAMPS
        AddPlugin("ptsnormalise","ptsnormalise_video");
        ConnectPipeline(*m_sourceBins,*m_sinkBin,"ptsnormalise_video");
 #else
  #ifdef _USE_BARRIER
        linked=ConnectPipeline(*m_sourceBins,*m_sinkBin,"barrier");
        // linked=gst_element_link_many(
        //     FindNamedPlugin(*m_sourceBins),
        //     FindNamedPlugin("barrier"),
        //     FindNamedPlugin(*m_sinkBin),
        //     NULL
        // );
  #else        
        ConnectPipeline(*m_sourceBins,*m_sinkBin);
  #endif
 #endif
#endif

        // now set up the pad block - the RTSP takes some time to start playing
        // if we send subs before it the runtime gets bent and join fails with decreasing timestamps
#ifdef _USE_PROBES
        attachProbes("nmeasource",GST_PAD_PROBE_TYPE_BLOCK,true,&m_nmea);
        // TODO when handling multiple video streams this needs to block all until all streams are producing
        attachProbes(*m_sourceBins,GST_PAD_PROBE_TYPE_BUFFER);
#endif

        return linked;
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
        // if(!m_dataFlowing)
        // {
        //     GstBuffer *theBuffer=gst_pad_probe_info_get_buffer(padinfo);
        //     GstEvent *gap=gst_event_new_gap(0,theBuffer->pts);
        //     if(!gst_pad_push_event(pad,gap))
        //     {
        //         GST_ERROR_OBJECT (m_pipeline, "gst_pad_push_event gap failed");
        //     }
        // }

        m_dataFlowing=true;
    }

    virtual void SplitMuxOpenedSplit(GstClockTime splitStart, const char*newFile)
    {
        m_currentChapterGuid=boost::uuids::random_generator()();

        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(newFile,
                                                        m_journeyGuid,
                                                        m_currentChapterGuid,
                                                        (splitStart/GST_MSECOND)));


        GST_ERROR_OBJECT (m_pipeline, "Opened split file - start %" GST_TIME_FORMAT " basetime %" GST_TIME_FORMAT "",
            GST_TIME_ARGS(splitStart),
            GST_TIME_ARGS(m_basetime)
            );

    }

    virtual void SplitMuxClosedSplit(GstClockTime splitEnd, const char*newFile)
    {
        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(m_journeyGuid,
                                                        m_currentChapterGuid,
                                                        (splitEnd/GST_MSECOND)));


        GST_ERROR_OBJECT (m_pipeline, "Closed split file");
        
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

    void Run()
    {
        // start a journey
        m_journeyGuid=boost::uuids::random_generator()();
        m_scheduler.m_taskQueue.safe_push(
            sqlWorkJobs(sqlWorkJobs::taskType::swjStartJourney,m_journeyGuid));

        // run
        gstreamPipeline::Run(60*3);

        // close a journey
        m_scheduler.m_taskQueue.safe_push(
            sqlWorkJobs(sqlWorkJobs::taskType::swjEndJourney,m_journeyGuid));

        // TODO fix this
        sleep(2);

        m_scheduler.stop();

    }

    // from pipeline
    virtual void pipelineStateChangeMessageHandler(GstMessage*msg)
    {
        GstState oldState, newState;
        gst_message_parse_state_changed(msg, &oldState, &newState, NULL);
        if(newState==GST_STATE_PLAYING)
        {
            m_basetime=gstreamPipeline::GetTimeSinceEpoch();
            m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(m_journeyGuid,m_basetime));
        }
    }


protected:

    bool m_browserDone, m_fatal;
    std::vector<std::string> arecords;
    
    multiRemoteSourceBin<rtspSourceBin> *m_sourceBins;
    gstSplitMuxOutBin *m_sinkBin;
    std::thread m_browser;

    std::string m_outspec;

    mariaDBconnection m_sql;

    sqlWorkerThread<sqlWorkJobs> m_scheduler;
    boost::uuids::uuid m_journeyGuid, m_currentChapterGuid;
    GstClockTime m_basetime;


#ifdef _USE_NMEA    
    gstNmeaToSubs m_nmea;
    volatile bool m_dataFlowing;
#endif    
};





int main()
{

    gstreamPipeline thePipeline("mainPipeline");
    ringBufferPipeline ringPipeline("/vids",2);

    ringPipeline.Run();

}
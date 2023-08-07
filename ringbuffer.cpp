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

#define _DEBUG_TIMESTAMPS
#ifdef _DEBUG_TIMESTAMPS
#include "gstreamHelpers/myplugins/gstptsnormalise.h"
#endif

#include "gstreamHelpers/helperBins/probeHelper.h"

#include "dashcam_sql.h"
#include "rb_tasks.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <signal.h>



//#define _USE_NMEA
//#define _USE_MDNS
//#define _USE_PROBES
//#define _USE_BARRIER

#define USE_RTSP

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
        m_dataFlowing(false),
#endif        
        padProber(this),
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

        strftime(buffer,sizeof(buffer)-1,"/%FT%TZ_%%05d.mp4",info);
        m_outspec+=buffer;


        m_sinkBin=new gstSplitMuxOutBin(this,sliceMins*60,m_outspec.c_str());


        // mark up the records
        // WIFI may cause video jitters

        //std::vector<std::string> urls={"rtsp://rpizerocam:8554/cam"};
#ifdef USE_RTSP
        std::vector<std::string> urls={"rtsp://vpnhack:8554/cam"};
#else
        std::vector<std::string> urls={"rtmp://vpnhack/cam"};
#endif        
        for(auto each=arecords.begin();each!=arecords.end();each++)
        {
            // rtsp://vpnhack:8554/cam
#ifdef USE_RTSP
            std::string url("rtsp://");
#else
            std::string url("rtmp://");
#endif            
            url+=*each;
#ifdef USE_RTSP
            url+=":8554/cam";
#else
            url+=":1935/cam";
#endif            
            urls.push_back(url);
        }

#ifdef USE_RTSP
        m_sourceBins=new multiRemoteSourceBin<rtspSourceBin>(this,urls,"video/x-h264,stream-format=(string)avc,alignment=(string)au");
#else
        m_sourceBins=new multiRemoteSourceBin<rtmpSourceBin>(this,urls,"video/x-h264,stream-format=(string)avc");
#endif

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
        bool linked=ConnectPipeline(m_nmea,*m_sinkBin,"ptsnormalise_subs")==0;
 #else
  #ifdef _USE_BARRIER
        //bool linked=ConnectPipeline(m_nmea,*m_sinkBin,"barrier");
          bool linked=gst_element_link_many(
            FindNamedPlugin(m_nmea),
            FindNamedPlugin("barrier"),
            FindNamedPlugin(*m_sinkBin),
            NULL
        );
  #else
        bool linked=(ConnectPipeline(m_nmea,*m_sinkBin)==0);
  #endif
 #endif        


 #ifdef _DEBUG_TIMESTAMPS
        AddPlugin("ptsnormalise","ptsnormalise_video");
        linked=ConnectPipeline(*m_sourceBins,*m_sinkBin,"ptsnormalise_video")==0;
 #else
  #ifdef _USE_BARRIER
        //linked=ConnectPipeline(*m_sourceBins,*m_sinkBin,"barrier");
        linked=gst_element_link_many(
            FindNamedPlugin(*m_sourceBins),
            FindNamedPlugin("barrier"),
            FindNamedPlugin(*m_sinkBin),
            NULL
        );
  #else        
        linked=(ConnectPipeline(*m_sourceBins,*m_sinkBin)==0);
  #endif
 #endif
#else 
        bool linked=(ConnectPipeline(*m_sourceBins,*m_sinkBin)==0);;
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

#ifdef _USE_NMEA    

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
#endif

    virtual void SplitMuxOpenedSplit(GstClockTime splitStart, const char*newFile)
    {
        this->DumpGraph(newFile);
        
        // it's possible that this comes in before the 'close chapter' call,
        // so don't change the guid until we know we're safe too
        //while(!m_chapter_open.try_lock_for(std::chrono::milliseconds(10)))
        while(m_chapter_open)
        {
            GST_WARNING_OBJECT (m_pipeline, "Failed to get 'm_chapter_open' mutex");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        m_currentChapterGuid=boost::uuids::random_generator()();

        m_chapter_open=true;

        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(newFile,
                                                        m_journeyGuid,
                                                        m_currentChapterGuid,
                                                        (splitStart/GST_MSECOND)));

        GST_INFO_OBJECT (m_pipeline, "Opened split file - chapter %s",boost::uuids::to_string(m_currentChapterGuid).c_str());
        GST_INFO_OBJECT (m_pipeline, "Opened split file - start %" GST_TIME_FORMAT " basetime %" GST_TIME_FORMAT "",
            GST_TIME_ARGS(splitStart),
            GST_TIME_ARGS(m_basetime)
            );

        printf("Opened split %s\r\n",newFile);

    }

    virtual void SplitMuxClosedSplit(GstClockTime splitEnd, const char*newFile)
    {
        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(m_journeyGuid,
                                                        m_currentChapterGuid,
                                                        (splitEnd/GST_MSECOND)));

        // and add a prune job
        // TODO use the proper time
        time_t rawtime;
        time(&rawtime);
        GstClockTime gtime=(rawtime-3600)*GST_SECOND;
        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(gtime));

        //m_chapter_open.unlock();
        m_chapter_open=false;

        GST_INFO_OBJECT (m_pipeline, "Closed split file - chapter %s",boost::uuids::to_string(m_currentChapterGuid).c_str());
        GST_INFO_OBJECT (m_pipeline, "Closed split file - end %" GST_TIME_FORMAT " basetime %" GST_TIME_FORMAT "",
            GST_TIME_ARGS(splitEnd),
            GST_TIME_ARGS(m_basetime)
            );

        printf("Closed split %s\r\n",newFile);

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

    void sliceNow()
    {
        if(m_sinkBin)
        {
            m_sinkBin->SendActionSignal("splitmuxsink","split-now");
        }
    }
    
    void Run(unsigned minutes=0)
    {
        // start a journey
        m_journeyGuid=boost::uuids::random_generator()();
        m_scheduler.m_taskQueue.safe_push(
            sqlWorkJobs(sqlWorkJobs::taskType::swjStartJourney,m_journeyGuid));

        // run
        gstreamPipeline::Run(minutes*60);

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
    
#ifdef USE_RTSP
    multiRemoteSourceBin<rtspSourceBin> *m_sourceBins;
#else
    multiRemoteSourceBin<rtmpSourceBin> *m_sourceBins;
#endif    
    gstSplitMuxOutBin *m_sinkBin;
    std::thread m_browser;
    // TODO when i get to std-v20 implement this
    // std::binary_semaphore m_chapter_open;
    volatile bool m_chapter_open=false;

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


volatile bool ctrlCseen=false;

void intHandler(int dummy) 
{
    printf("Handling ctrl-C\n\r");
    ctrlCseen = true;
}


ringBufferPipeline ringPipeline("/vids",15);

void sliceNow(int)
{
    ringPipeline.sliceNow();    
}

int main()
{
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    signal(SIGHUP, sliceNow);

    //gstreamPipeline thePipeline("mainPipeline");
    // slice every 15 mins
    ringPipeline.setExitVar(&ctrlCseen);
    ringPipeline.Run();

}
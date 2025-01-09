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
#include "avahi_helper.h"

#include "gstreamHelpers/helperBins/probeHelper.h"

#include "dashcam_sql.h"
#include "rb_tasks.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <signal.h>

#define USE_RTSP

// sudo setcap cap_net_admin=eip ./ringbuffer

//#define _DEBUG_NO_SQL   // don't pollute DB while testing

//#define _NTP_SOURCE "burner"

class ringBufferPipeline : 
    public gstreamPipeline, 
    public padProber,
    public avahiHelper
{


public:
    ringBufferPipeline(const char *outdir, unsigned sliceMins=15):
        avahiHelper("_dashcam._tcp"),
        //avahiHelper("_barneyman._tcp"),
        gstreamPipeline("ringBufferPipeline"),
        m_sourceBins(NULL),
        m_browserDone(false),
        m_fatal(false),
        m_nmea(this),
        m_mq(this),
        padProber(this),
        m_sql("debian12","dashcam","dashcam","dashcam")
    {
        // use NTP clock
        //UseNTPv4Clock();

        while(!avahi_finished())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

#ifdef _NTP_SOURCE
        UseNTPv4Clock(_NTP_SOURCE);
#else        
        // use real time from the system clock
        setRealtimeClock();
#endif        

        char buffer[300];
        struct tm *info; 
        time_t now=time(NULL);
        info = gmtime(&now);

        m_outspec=outdir;

        strftime(buffer,sizeof(buffer)-1,"/%FT%TZ_%%05d.mp4",info);
        m_outspec+=buffer;

        if(m_servicesFound.size())
        {
            std::vector<std::string> urls;

            m_sinkBin=new gstSplitMuxOutBin(this,sliceMins*60,m_outspec.c_str());

            // sort them by priority
            std::sort(m_servicesFound.begin(),m_servicesFound.end(),
                [](const std::tuple<std::string,std::string,std::vector<std::string>> &a,const std::tuple<std::string,std::string,std::vector<std::string>> &b){

                // need priority - lazy, drunk sort
                // TODO - fix this
                return a<b;

            });

            for(auto each=m_servicesFound.begin();each!=m_servicesFound.end();each++)
            {
                // rtsp://vpnhack:8554/cam
    #ifdef USE_RTSP
                std::string url("rtsp://");
    #else
                std::string url("rtmp://");
    #endif            
                url+=std::get<1>(*each);
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

            m_fatal=!buildPipeline();
        }
        else
        {
            m_fatal=true;
        }
    }

    ~ringBufferPipeline()
    {
        releaseMyRequestedPads();
        delete m_sinkBin;
        delete m_sourceBins;
    }




    bool buildPipeline()
    {

        bool linked=(ConnectPipeline(m_nmea,*m_sinkBin,m_mq)==0);
        linked=(ConnectPipeline(*m_sourceBins,*m_sinkBin,m_mq)==0);

        return linked;
    }


    virtual void SplitMuxOpenedSplit(GstClockTime splitStart, const char*newFile)
    {
        //this->DumpGraph(boost::uuids::to_string(m_currentChapterGuid).c_str());
        
        // it's possible that this comes in before the 'close chapter' call,
        // so don't change the guid until we know we're safe too
        while(m_chapter_open)
        {
            GST_WARNING_OBJECT (m_pipeline, "Failed to get 'm_chapter_open' mutex");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        m_currentChapterGuid=boost::uuids::random_generator()();

        m_chapter_open=true;

#ifndef _DEBUG_NO_SQL
        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(newFile,
                                                        m_journeyGuid,
                                                        m_currentChapterGuid,
                                                        (splitStart/GST_MSECOND)));
#endif

        GST_INFO_OBJECT (m_pipeline, "Opened split file - chapter %s",boost::uuids::to_string(m_currentChapterGuid).c_str());
        GST_INFO_OBJECT (m_pipeline, "Opened split file - start %" GST_TIME_FORMAT " basetime %" GST_TIME_FORMAT "",
            GST_TIME_ARGS(splitStart),
            GST_TIME_ARGS(m_basetime)
            );

        printf("Opened split %s\r\n",newFile);

    }

    virtual void SplitMuxClosedSplit(GstClockTime splitEnd, const char*newFile)
    {

#ifndef _DEBUG_NO_SQL
        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(m_journeyGuid,
                                                        m_currentChapterGuid,
                                                        (splitEnd/GST_MSECOND)));
#endif

        // and add a prune job
        // TODO use the proper time
        time_t rawtime;
        time(&rawtime);
        GstClockTime gtime=(rawtime-(86400*1))*GST_SECOND;

#ifndef _DEBUG_NO_SQL
        m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(gtime));
#endif 

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
        if(m_fatal)
        {
            return;
        }
        // start a journey
        m_journeyGuid=boost::uuids::random_generator()();
#ifndef _DEBUG_NO_SQL
        m_scheduler.m_taskQueue.safe_push(
            sqlWorkJobs(sqlWorkJobs::taskType::swjStartJourney,m_journeyGuid));
#endif
        // run
        gstreamPipeline::Run(minutes*60);

#ifndef _DEBUG_NO_SQL
        // close a journey
        m_scheduler.m_taskQueue.safe_push(
            sqlWorkJobs(sqlWorkJobs::taskType::swjEndJourney,m_journeyGuid));
#endif

        m_scheduler.stop();

    }

    // from pipeline
    virtual void pipelineStateChangeMessageHandler(GstState old_state,GstState new_state, GstState pendingState)
    {
        // must call down
        gstreamPipeline::pipelineStateChangeMessageHandler(old_state,new_state,pendingState);

        if(new_state==GST_STATE_PLAYING)
        {
            m_basetime=gstreamPipeline::GetTimeSinceEpoch();
#ifndef _DEBUG_NO_SQL
            m_scheduler.m_taskQueue.safe_push(sqlWorkJobs(m_journeyGuid,m_basetime));
#endif            
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
    // TODO when i get to std-v20 implement this
    // std::binary_semaphore m_chapter_open;
    volatile bool m_chapter_open=false;

    std::string m_outspec;

    mariaDBconnection m_sql;

    sqlWorkerThread<sqlWorkJobs> m_scheduler;
    boost::uuids::uuid m_journeyGuid, m_currentChapterGuid;
    GstClockTime m_basetime;
    gstMultiQueueBin m_mq;
    gstNmeaToSubs m_nmea;
};


volatile bool ctrlCseen=false;

void intHandler(int dummy) 
{
    printf("Handling ctrl-C\n\r");
    ctrlCseen = true;
}

#define _DEBUG

#ifdef _DEBUG
ringBufferPipeline ringPipeline("/vids",1);
#else
ringBufferPipeline ringPipeline("/vids",5);
#endif

void sliceNow(int)
{
    ringPipeline.sliceNow();    
}

int main()
{
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    signal(SIGHUP, sliceNow);

    ringPipeline.setExitVar(&ctrlCseen);

#ifdef _DEBUG
    ringPipeline.Run(3);
#else
    ringPipeline.Run();
#endif

}
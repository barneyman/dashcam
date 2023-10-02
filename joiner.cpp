#include <thread>
#include <chrono>
#include <vector>

#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"
#include "gstreamHelpers/helperBins/myDemuxBins.h"

#include "metaBins.h"

#include <dirent.h> 

#define USE_PANGO _PANGO_SPEED | _PANGO_LONGLAG | _PANGO_UTC
//#define _USE_GLIMAGE
#define _USE_NAMED_VIDEO


class joinVidsPipeline : public gstreamPipeline
{
public:
    joinVidsPipeline(std::vector<std::string> &files, const char*destination,
#ifdef _USE_NAMED_VIDEO    
                        const char* name,
#endif
                        GstClockTime offset=GST_CLOCK_TIME_NONE, 
                        GstClockTime runtime=GST_CLOCK_TIME_NONE):
        gstreamPipeline("joinVidsPipeline"),
        m_multisrc(this,files,offset,runtime),
        m_mixer(this, false),
        m_meta(this, USE_PANGO ),
#ifdef _USE_GLIMAGE
        m_imageSink(this),
#endif
#ifdef _USE_NAMED_VIDEO    
        m_name(this,name,"debugName",4),
#endif
        m_out(this,destination)

    {

        if(m_multisrc.fatalError())
        {
            m_fatal=true;
            return;
        }


        // connect h264
        ConnectPipeline(m_mixer,m_meta);
#ifdef _USE_GLIMAGE
        {
            ConnectPipeline(m_meta,m_out,m_imageSink);
        }
#else // _USE_GLIMAGE
        {
            ConnectPipeline(m_meta,
                            m_out
#ifdef _USE_NAMED_VIDEO    
                            ,m_name
#endif
                            );    
            DumpGraph("h264-2");
        }
#endif // _USE_GLIMAGE

        // connect h264
        ConnectPipeline(m_multisrc,m_mixer);
        if(m_multisrc.streamInfo.numSubtitleStreams())
        {
            // connect subs
            ConnectPipeline(m_multisrc,m_meta);
        }
        else
        {
            m_meta.turnOffWaitText("no meta");
        }

    }

    ~joinVidsPipeline()
    {
        releaseMyRequestedPads();
    }

    operator bool() { return !m_fatal; }

protected:

    gstMP4DemuxDecodeSparseBin m_multisrc;
    gstVideoMixerBin m_mixer;

    gstMP4OutBin m_out;
    gstMultiNmeaJsonToPangoRenderBin m_meta;

#ifdef _USE_GLIMAGE
    gstValve<gstGLImageSink> m_imageSink;
#endif
#ifdef _USE_NAMED_VIDEO    
    gstNamedVideo m_name;
#endif

    bool m_fatal=false;
};


std::vector<std::string> collectFiles(const char*dirname)
{
    std::vector<std::string> files;

    DIR *d;
    struct dirent *dir;
    d = opendir(dirname);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            //printf("%s\n", dir->d_name);
            if(dir->d_type == DT_REG)
            {
                std::string newFile(dirname);
                newFile+=dir->d_name;
                files.push_back(std::string(newFile));
            }
        }
        closedir(d);

        std::sort(files.begin(),files.end());

    }
    return files;       
}

volatile bool ctrlCseen=false;

void intHandler(int dummy) 
{
    printf("Handling ctrl-C - pls wait ...\n\r");
    ctrlCseen = true;
}

#define USE_SQL

#ifdef USE_SQL
#include "jn_tasks.h"
#endif

int main()
{

    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

#ifdef USE_SQL

    mariaDBconnection sql("debian","dashcam","dashcam","dashcam");

    if(!sql)
    {
        printf("SQL error: %s\n\r",sql.error());
        exit(1);
    }

    // just stop ref count dropping out
    gstreamPipeline sentry("Sentry");


    GstClockTime offsetms, lengthms;
    maria_guid id;

    while(!ctrlCseen)
    {
        grabber theGrabber(sql);

        unsigned long count=0;

        while(theGrabber.isPopulated() && !ctrlCseen)
        {
            maria_timestamp timein, timeout;
            auto filelist=theGrabber.getGrabDetails(offsetms,lengthms,id,timein, timeout);

            if(filelist.size())
            {

                std::string filename="/vids/grabs/"+id.to_string()+".mp4";

                printf("%lu/%lu : %lu files -> %s for %f s\n\r",++count,theGrabber.size(),filelist.size(),filename.c_str(), (float)(lengthms/1000));

                joinVidsPipeline jpl(filelist,
                                            filename.c_str(),
#ifdef _USE_NAMED_VIDEO
                                            (timein.to_string()+std::string("-")+timeout.to_string()).c_str(),
#endif
                                            offsetms*GST_MSECOND,
                                            (offsetms+lengthms)*GST_MSECOND
                                            );
                if(!jpl)
                {
                    printf("joinVidsPipeline ctor failed\n\r");
                    theGrabber.updateGrabDetails(id,-2,"*** joinVidsPipeline ctor failed");

                }
                else
                {
                    jpl.setExitVar(&ctrlCseen);
                    if(jpl.Run())
                    {
                        theGrabber.updateGrabDetails(id,0,filename);
                    }
                    else
                    {
                        theGrabber.updateGrabDetails(id,-3,"Run error");
                    }

                }

                printf("**********************\n\r");

            }
            else
            {
                printf("no file list\n\r");
                theGrabber.updateGrabDetails(id,-1,"*** No file list");                
            }

        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

#else


    std::vector<std::string> files=collectFiles("/vids/");

    if(!files.size())
    {
        return -1;
    }

//    joinVidsPipeline joiner(files,"/workspaces/dashcam/combined.mp4",90*GST_SECOND,100*GST_SECOND);
    joinVidsPipeline joiner(files,"/workspaces/dashcam/combined.mp4");
    joiner.setExitVar(&ctrlCseen);

//    joiner.Run();
    joiner.Run(10);
#endif

}
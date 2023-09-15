#include <thread>
#include <chrono>
#include <vector>

#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"
#include "gstreamHelpers/helperBins/myDemuxBins.h"

#include "metaBins.h"

#include <dirent.h> 


#define _USE_PANGO
#define _USE_MULTI

class joinVidsPipeline : public gstreamPipeline
{
public:
    joinVidsPipeline(std::vector<std::string> &files, const char*destination,GstClockTime offset=GST_CLOCK_TIME_NONE, GstClockTime runtime=GST_CLOCK_TIME_NONE):
        gstreamPipeline("joinVidsPipeline"),
#ifdef _USE_MULTI        
        m_multisrc(this,files,offset,runtime),
        m_mixer(this),
#else        
        m_src(this,files[0].c_str()),
#endif        
#ifdef _USE_PANGO
        //m_meta(this, _PANGO_SPEED | _PANGO_LONGLAG | _PANGO_UTC),
        m_meta(this, _PANGO_LONGLAG ),
#endif        
        m_out(this,destination)

    {
#ifdef _USE_PANGO
#ifdef _USE_MULTI        

        //ConnectPipeline(m_multisrc,m_out,m_meta);



        ConnectPipeline(m_mixer,m_out,m_meta);
        ConnectPipeline(m_multisrc,m_mixer);
        ConnectPipeline(m_multisrc,m_meta);

        // need to handle subtitles turning up for the compositor
        // ConnectPipeline(m_multisrc,m_mixer);
        // ConnectPipeline(m_mixer,m_out,m_meta);
#else
        ConnectPipeline(m_src,m_out,m_meta);
#endif        
#else
#ifdef _USE_MULTI        
        ConnectPipeline(m_multisrc,m_out);
#else
        ConnectPipeline(m_src,m_out);
#endif        
#endif        
    }

    ~joinVidsPipeline()
    {
        releaseMyRequestedPads();
    }

protected:

#ifdef _USE_MULTI        
    gstMP4DemuxDecodeSparseBin m_multisrc;
    gstVideoMixerBin m_mixer;
#else    
    gstMP4DemuxDecodeBin m_src;
#endif    

    gstMP4OutBin m_out;
#ifdef _USE_PANGO
    gstMultiNmeaJsonToPangoRenderBin m_meta;
#endif    
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
    printf("Handling ctrl-C\n\r");
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

    // just stop ref count dropping out
    gstreamPipeline sentry("Sentry");

    grabber theGrabber(sql);

    GstClockTime offsetms, lengthms;
    maria_guid id;

    while(!ctrlCseen)
    {
        //auto filelist=theGrabber.getGrabDetails(offsetms,lengthms,id);
        while(theGrabber.isPopulated())
        {
            auto filelist=theGrabber.getGrabDetails(offsetms,lengthms,id);

            if(filelist.size())
            {

                std::string filename="/vids/grabs/"+id.to_string()+".mp4";

                printf("-> %s for %f s\n\r",filename.c_str(), (float)(lengthms/1000));

                if(joinVidsPipeline(filelist,
                                            filename.c_str(),
                                            offsetms*GST_MSECOND,
                                            (offsetms+lengthms)*GST_MSECOND
                                            ).Run())
                {
                    theGrabber.updateGrabDetails(id,0,filename);
                }

                printf("**********************\n\r");

            }

        }
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }

#else


#ifdef _USE_MULTI        
    std::vector<std::string> files=collectFiles("/vids/");
#else
    std::vector<std::string> files={"/vids/2023-03-060828Z_00000.mp4"};
#endif

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
#include <thread>
#include <chrono>
#include <vector>

#include "gstreamHelpers/helperBins/myElementBins.h"
#include "gstreamHelpers/helperBins/myMuxBins.h"
#include "gstreamHelpers/helperBins/myDemuxBins.h"

#include "metaBins.h"

#include "mdns_cpp/mdns.hpp"
#include "mdns_cpp/macros.hpp"
#include "mdns_cpp/utils.hpp"

#include <dirent.h> 


#define _USE_PANGO
#define _USE_MULTI

class joinVidsPipeline : public gstreamPipeline
{
public:
    joinVidsPipeline(std::vector<std::string> &files, const char*destination):
        gstreamPipeline("joinVidsPipeline"),
#ifdef _USE_MULTI        
        m_multisrc(this,files),
#else        
        m_src(this,files[0].c_str()),
#endif        
#ifdef _USE_PANGO
        m_meta(this),
#endif        
        m_out(this,destination)

    {
#ifdef _USE_PANGO
#ifdef _USE_MULTI        
        ConnectPipeline(m_multisrc,m_out,m_meta);
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

protected:

#ifdef _USE_MULTI        
    gstMP4DemuxDecodeSparseBin m_multisrc;
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


int main()
{

    //gstreamDemuxExamineDiscrete tester("/vids/2023-02-262220_00004.mp4","qtdemux");

#ifdef _USE_MULTI        
    std::vector<std::string> files=collectFiles("/vids/");
#else
    std::vector<std::string> files={"/vids/2023-03-060828Z_00000.mp4"};
#endif
    joinVidsPipeline joiner(files,"/workspaces/dashcam/combined.mp4");
    joiner.Run();


}
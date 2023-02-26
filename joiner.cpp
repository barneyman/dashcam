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



class joinVidsPipeline : public gstreamPipeline
{
public:
    joinVidsPipeline(std::vector<std::string> &files, const char*destination):
        gstreamPipeline("joinVidsPipeline"),
        m_multisrc(this,files),
        m_meta(this),
        m_out(this,destination)

    {
        ConnectPipeline(m_multisrc,m_out,m_meta);
    }

protected:

        gstMP4DemuxDecodeSparseBin m_multisrc;
        gstMP4OutBin m_out;
        gstMultiNmeaJsonToPangoRenderBin m_meta;
};


int main()
{

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


}
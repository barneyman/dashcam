#include "gstreamHelpers/helperBins/myJsonBins.h"
#include "gstreamHelpers/myplugins/gstnmeasource.h"

// turns an nmea json into speed pango
class gstJsonNMEAtoSpeed : public gstJsonToPangoBin
{
public:    
    gstJsonNMEAtoSpeed(pluginContainer<GstElement> *parent):gstJsonToPangoBin(parent)
    {
    }

    virtual std::string TurnJsonToPango(nlohmann::json &jsondata,GstBuffer *)
    {
        char msg[PANGO_BUFFER];
        int len=0;

        if(jsondata.contains("speedKMH") && jsondata.contains("bearingDeg") && jsondata.contains("satelliteCount"))
        {

            len=snprintf(msg, sizeof(msg), "<span foreground=\"white\" size=\"small\">%d km/h %.1f° %2d sats</span>",
                jsondata["speedKMH"].get<int>(),
                jsondata["bearingDeg"].get<float>(),
                jsondata["satelliteCount"].get<int>());

            return msg;  
        }

        return "Subtitle Error";
    }
};

// hijacks the whole framework to just add timestamp pango
class gstJsonFrameNumber : public gstJsonToPangoBin
{
public:    
    gstJsonFrameNumber(pluginContainer<GstElement> *parent):gstJsonToPangoBin(parent)
    {
    }

    
    virtual std::string TurnJsonToPango(nlohmann::json &,GstBuffer *inbuf)
    {
        char msg[PANGO_BUFFER];
        int len=0;

        len=snprintf(msg, sizeof(msg), "<span foreground=\"white\" size=\"small\">DTS %" GST_TIME_FORMAT " PTS %" GST_TIME_FORMAT " Dur %" GST_TIME_FORMAT "</span>",
        //len=snprintf(msg, sizeof(msg), "<span foreground=\"white\" size=\"small\">Frame %lu DTS %" GST_TIME_FORMAT " PTS %" GST_TIME_FORMAT " Dur %" GST_TIME_FORMAT "</span>",
        //    inbuf->offset,
            GST_TIME_ARGS(inbuf->dts),
            GST_TIME_ARGS(inbuf->pts),
            GST_TIME_ARGS(inbuf->duration)
            );

        return msg;  
    }
};



// reads NMEA from json and turns it into long lat pango
class gstJsonNMEAtoLongLat : public gstJsonToPangoBin
{
public:    
    gstJsonNMEAtoLongLat(pluginContainer<GstElement> *parent):gstJsonToPangoBin(parent)
    {
    }

    virtual std::string TurnJsonToPango(nlohmann::json &jsondata,GstBuffer *)
    {
        char msg[PANGO_BUFFER];
        int len=0;

        if(jsondata.contains("longitudeE") && jsondata.contains("latitudeN"))
        {

            len=snprintf(msg, sizeof(msg), "<span foreground=\"white\" size=\"small\">Long:%.4f Lat:%.4f</span>",
                jsondata["longitudeE"].get<float>(),
                jsondata["latitudeN"].get<float>()
                );

            return msg;  
        }

        return "Subtitle Error";
    }
};

// reads NMEA from json and turns it into utc pango
class gstJsonNMEAtoUTC : public gstJsonToPangoBin
{
public:    
    gstJsonNMEAtoUTC(pluginContainer<GstElement> *parent):gstJsonToPangoBin(parent)
    {
    }

    virtual std::string TurnJsonToPango(nlohmann::json &jsondata,GstBuffer *)
    {
        char msg[PANGO_BUFFER];
        int len=0;

        std::string output;

        if(jsondata.contains("utcsecs"))
        {
            /// oooh - we can localise!
            time_t nowsecs=jsondata["utcsecs"];
            struct tm *info = localtime(&nowsecs);

            time_t millis=0;
            if(jsondata.contains("utcmillis"))
                millis=jsondata["utcmillis"];

            snprintf(msg,sizeof(msg)-1, "%d-%02d-%02d %02d:%02d:%02d.%03lu %s",
                info->tm_year+1900,
                info->tm_mon+1,
                info->tm_mday,
                info->tm_hour,
                info->tm_min,
                info->tm_sec,
                millis,
                tzname[info->tm_isdst]
                );

            output=msg;


        }

        else if(jsondata.contains("utc"))
        {
            output=jsondata["utc"].get<std::string>();
        }
        else
        {
            output="Subtitle error";
        }

        len=snprintf(msg, sizeof(msg), "<span foreground=\"white\" size=\"small\">%s</span>",
            output.c_str()
            );

        return msg;
    }
};





#define _PANGO_SPEED    1
#define _PANGO_LONGLAG  2
#define _PANGO_UTC      4
#define _PANGO_FRAME    8
class gstMultiNmeaJsonToPangoRenderBin : public gstMultiJsonToPangoRenderBin
{
public:
    gstMultiNmeaJsonToPangoRenderBin(pluginContainer<GstElement> *parent, unsigned mask = 0x0f):
        gstMultiJsonToPangoRenderBin(parent)
    {
        if(mask&_PANGO_SPEED)
            add<gstJsonNMEAtoSpeed>("pangoBin1", 2, 1);
        if(mask&_PANGO_LONGLAG)
            add<gstJsonNMEAtoLongLat>("pangoBin2", 0, 1);
        if(mask&_PANGO_UTC)
            add<gstJsonNMEAtoUTC>("pangoBin3", 1, 2);
        if(mask&_PANGO_FRAME)
            add<gstJsonFrameNumber>("pangobin4", 1, 4);

        finished();
    }

};


// this is not a use case - it was a test bin
class gstNmeaSourceWithOverlayOutput : public gstreamBin
{
public:
    gstNmeaSourceWithOverlayOutput(pluginContainer<GstElement> *parent):
        gstreamBin("gstNmeaSourceWithOverlayOutput",parent),
        m_queue(this,"q"),
        m_renderText(this)   
    {
        gst_nmeasource_registerRunTimePlugin();
        pluginContainer<GstElement>::AddPlugin("nmeasource");

        gst_element_link_many(   pluginContainer<GstElement>::FindNamedPlugin("nmeasource"),
                            pluginContainer<GstElement>::FindNamedPlugin(m_queue),        
                            pluginContainer<GstElement>::FindNamedPlugin(m_renderText),
                            NULL);        

        // ghost the video in and video out
        AddGhostPads(m_renderText,m_renderText);

        setBinFlags(GST_ELEMENT_FLAG_SOURCE);
    }

protected:
    gstMultiNmeaJsonToPangoRenderBin m_renderText;
    gstQueue2 m_queue;

};

class gstNmeaToSubs : public gstreamBin
{
public:
    gstNmeaToSubs(gstreamPipeline *parent):
        gstreamBin("nmeasourcebin", parent),
        m_queue(this,"q")
    {
        gst_nmeasource_registerRunTimePlugin();
        pluginContainer<GstElement>::AddPlugin("nmeasource");

        g_object_set(pluginContainer<GstElement>::FindNamedPlugin("nmeasource"),
            "parent",parent,
            "localtime", true,
            NULL);

        gst_element_link_many(   pluginContainer<GstElement>::FindNamedPlugin("nmeasource"),
                            pluginContainer<GstElement>::FindNamedPlugin(m_queue),        
                            NULL);        

        AddGhostPads(NULL,m_queue);

        setBinFlags(GST_ELEMENT_FLAG_SOURCE);

    }

    

protected:
    gstQueue2 m_queue;

};
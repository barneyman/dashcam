#include <libgpsmm.h>
#include "gstreamHelpers/json/json.hpp"

int main(void)
{
    struct gps_data_t *gps_data;

    gpsmm gps_rec("burner", DEFAULT_GPSD_PORT);

    if (gps_rec.stream(WATCH_ENABLE|WATCH_JSON) == NULL) {
        return 0;
    }

    unsigned sats_visible=0, sats_used=0;

    while(true)
    {
        nlohmann::json jsonData;

        if (!gps_rec.waiting(50000000))
            continue;

        if ((gps_data = gps_rec.read()) == NULL) 
        {
            continue;
        } 
        else 
        {

            if(!(gps_data->set&PACKET_SET))
            {
                continue;
            }

            //PROCESS(newdata);
            switch(gps_data->fix.mode)
            {
                case MODE_NOT_SEEN:
                    jsonData["msg"]="no sky";
                    sats_visible=0, sats_used=0;
                    break;
                case MODE_NO_FIX:
                    jsonData["msg"]="not fixed";
                    sats_visible=0, sats_used=0;
                    break;
                case MODE_3D:
                    if(gps_data->set & ALTITUDE_SET)
                        jsonData["altitudeM"]=gps_data->fix.altitude;
                    if(gps_data->set & CLIMB_SET)
                        jsonData["climb"]=gps_data->fix.climb;
                case MODE_2D:
                    
                    if(gps_data->set & LATLON_SET)
                    {
                        jsonData["longitudeE"]=trunc(gps_data->fix.longitude*10000)/10000.0;
                        jsonData["latitudeN"]=trunc(gps_data->fix.latitude*10000)/10000.0;
                    }
                    if(gps_data->set & SATELLITE_SET && gps_data->satellites_used)
                        sats_used=gps_data->satellites_used;

                    if(gps_data->set & SATELLITE_SET && gps_data->satellites_visible)
                        sats_visible=gps_data->satellites_visible;

                    if(gps_data->set & TRACK_SET)
                        jsonData["bearingDeg"]=(int)(gps_data->fix.track);
                    if(gps_data->set & SPEED_SET)
                        jsonData["speedKMH"]=(int)(gps_data->fix.speed/1000);

                    jsonData["set"]=gps_data->set;

                    break;


            }

            jsonData["satellitesUsed"]=sats_used;
            jsonData["satellitesVisible"]=sats_visible;


            if(gps_data->set&UNION_SET == UNION_SET)
                jsonData["status"]="hoopy";

            std::string jd=jsonData.dump();
            printf("%08lx %s\n\r",gps_data->set,jd.c_str());
            gps_data->set=0;
        }
    }

    return 1;

}
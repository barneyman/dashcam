#include <libgpsmm.h>
#include "gstreamHelpers/json/json.hpp"

int main(void)
{
    struct gps_data_t *gps_data;

    gpsmm gps_rec("burner", DEFAULT_GPSD_PORT);

    if (gps_rec.stream(WATCH_ENABLE|WATCH_JSON) == NULL) {
        return 0;
    }

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
            //PROCESS(newdata);
            switch(gps_data->fix.mode)
            {
                case MODE_NOT_SEEN:
                    jsonData["msg"]="no sky";
                    break;
                case MODE_NO_FIX:
                    jsonData["msg"]="not fixed";
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
                    if(gps_data->set & SATELLITE_SET)
                        jsonData["satelliteCount"]=gps_data->satellites_visible;

                    if(gps_data->set & TRACK_SET)
                        jsonData["bearingDeg"]=(int)(gps_data->fix.track);
                    if(gps_data->set & SPEED_SET)
                        jsonData["speedKMH"]=(int)(gps_data->fix.speed/1000);
                    break;


            }
            std::string jd=jsonData.dump();
            printf("%s\n\r",jd.c_str());
            gps_data->set=0;
        }
    }

    return 1;

}
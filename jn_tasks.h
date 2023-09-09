#include "dashcam_sql.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <queue>

class grabber
{
protected:

    mariaDBconnection *m_sql;

public:
    grabber(mariaDBconnection &sql)
    {
        m_sql=&sql;
        fetchGrabs();
    }

    bool isPopulated() 
    { 
        return m_grabsToGet.size()>0; 
    }

    std::vector<std::string> getGrabDetails(GstClockTime &offsetms,GstClockTime &lengthms, maria_guid &id)
    {
        std::vector<std::string> m_chapters;

        if(isPopulated())
        {
            std::tuple<maria_guid,maria_timestamp,maria_timestamp> firstGrab=m_grabsToGet.front();

            id=std::get<0>(firstGrab);

            mariaBinding<chapter_list,3> fetchChapterParams;
            mySQLprepared<chapter_list,3> callFetchChapter(m_sql, "CALL sp_get_chapter_list(?,?)");

            fetchChapterParams.m_timeIn=std::get<1>(firstGrab);
            fetchChapterParams.m_timeOut=std::get<2>(firstGrab);

            if(callFetchChapter.bind(fetchChapterParams))
            {
                offsetms=GST_CLOCK_TIME_NONE;

                if(callFetchChapter.execAndFetch(fetchChapterParams,[&](int flags)
                {
                    m_chapters.push_back(fetchChapterParams.m_grabfilename);
                    // take first one only
                    if(offsetms==GST_CLOCK_TIME_NONE)
                    {
                        offsetms=fetchChapterParams.m_offsetms;
                        lengthms=fetchChapterParams.m_lengthms;
                    }
                    printf("\t%s\n\r",fetchChapterParams.m_grabfilename);
                }))
                {
                }


            }

            m_grabsToGet.pop();                

        }
        return m_chapters;

    }


    void updateGrabDetails(maria_guid id, int result, std::string filename)
    {
        mariaBinding<update_grab,3> updateGrabParms;
        mySQLprepared<update_grab,3> callUpdateGrab(m_sql,"CALL sp_update_requested_grabs(?,?,?)");

        updateGrabParms.m_grabid=id;
        updateGrabParms.m_result=result;
        updateGrabParms.m_grabfilename=filename;

        if(callUpdateGrab.bind(updateGrabParms))
        {
            if(callUpdateGrab.exec())
            {

            }
            else
            {
                printf("%s\n\r",callUpdateGrab.error());
            }
        }
        else
        {
            printf("%s\n\r",callUpdateGrab.error());
        }





    }

protected:
    void fetchGrabs()
    {
        mariaBinding<get_requested_grabs,3> getGrabParams;
        mySQLprepared<get_requested_grabs,3> callgetGrabs(m_sql,"CALL sp_get_requested_grabs()");

        if(callgetGrabs.bind(getGrabParams))
        {
            if(callgetGrabs.execAndFetch(getGrabParams,[&](int flags)
                    {
                        m_grabsToGet.push(std::tuple<maria_guid,maria_timestamp,maria_timestamp>(   getGrabParams.m_grabid,
                                                                                                        getGrabParams.m_timeIn,
                                                                                                        getGrabParams.m_timeOut));
                    }))
            {
            }
        }
    }  

    std::queue<std::tuple<maria_guid,maria_timestamp,maria_timestamp>> m_grabsToGet;

};
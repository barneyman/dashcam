#include <stdio.h>
#include "dashcam_sql.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "rb_tasks.h"

void testgrabs(mariaDBconnection &m_sql)
{
    mariaBinding<get_requested_grabs,3> getGrabParams;
    mySQLprepared<get_requested_grabs,3> callgetGrabs(&m_sql,"CALL sp_get_requested_grabs()");

    std::vector<std::tuple<maria_guid,maria_timestamp,maria_timestamp>> grabsToGet;

    if(callgetGrabs.bind(getGrabParams))
    {
        if(callgetGrabs.execAndFetch(getGrabParams,[&](int flags)
                {
                    grabsToGet.push_back(std::tuple<maria_guid,maria_timestamp,maria_timestamp>(   getGrabParams.m_grabid,
                                                                                                      getGrabParams.m_timeIn,
                                                                                                      getGrabParams.m_timeOut));
                }))
        {
            printf("Got %lu grabs\n\r", grabsToGet.size());

            // now iterate thru them
            for(auto each=grabsToGet.begin();each!=grabsToGet.end();each++)
            {


                mariaBinding<chapter_list,3> fetchChapterParams;
                mySQLprepared<chapter_list,3> callFetchChapter(&m_sql, "CALL sp_get_chapter_list(?,?)");

                fetchChapterParams.m_timeIn=std::get<1>(*each);
                fetchChapterParams.m_timeOut=std::get<2>(*each);

                if(callFetchChapter.bind(fetchChapterParams))
                {
                    std::vector<std::string> m_chapters;
                    GstClockTime offsetms=GST_CLOCK_TIME_NONE, lengthms;

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

                        // .. do something
                        mariaBinding<update_grab,3> updateGrabParms;
                        mySQLprepared<update_grab,3> callUpdateGrab(&m_sql,"CALL sp_update_requested_grabs(?,?,?)");

                        updateGrabParms.m_grabid=std::get<0>(*each);
                        updateGrabParms.m_result=0;
                        updateGrabParms.m_grabfilename=std::get<0>(*each).to_string();

                        if(callUpdateGrab.bind(updateGrabParms))
                        {
                            if(callUpdateGrab.exec())
                            {
                                printf("%s\n\r",std::get<0>(*each).to_string().c_str());

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
                    else
                    {
                        printf("%s\n\r",callFetchChapter.error());
                    }
                }






            }
        }

    }

}

int main()
{

    mariaDBconnection sql("debian12","dashcam","dashcam","dashcam");

    if(!sql.valid())
    {
        printf("SQL error: %s\r\n",sql.error());
    }

    testgrabs(sql);

    return 0;

    boost::uuids::uuid testGuid=boost::uuids::random_generator()();

    sqlWorkerThread<sqlWorkJobs> scheduler;

    time_t rawtime;
    time(&rawtime);

    GstClockTime gtime=(rawtime-3600)*GST_SECOND;


    scheduler.m_taskQueue.safe_push(sqlWorkJobs(gtime));

    sleep(20);

    scheduler.m_taskQueue.safe_push(sqlWorkJobs(sqlWorkJobs::taskType::swjStartJourney,testGuid));

    sleep(2);


    scheduler.m_taskQueue.safe_push(sqlWorkJobs(testGuid,gtime));


    sleep(1);
    boost::uuids::uuid chapterGuid=boost::uuids::random_generator()();
    scheduler.m_taskQueue.safe_push(sqlWorkJobs("wibble",
                                                    testGuid,
                                                    chapterGuid,
                                                    500));

    sleep(1);
    scheduler.m_taskQueue.safe_push(sqlWorkJobs(testGuid,
                                                    chapterGuid,
                                                    5000));

    sleep(1);
    scheduler.m_taskQueue.safe_push(sqlWorkJobs(sqlWorkJobs::taskType::swjEndJourney,testGuid));

    sleep(1);



    scheduler.stop();

    return 0;

}
#include <stdio.h>
#include "dashcam_sql.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "rb_tasks.h"

int main()
{

    mariaDBconnection sql("debian","dashcam","dashcam","dashcam");

    if(!sql.valid())
    {
        printf("SQL error: %s\r\n",sql.error());
    }

    boost::uuids::uuid testGuid=boost::uuids::random_generator()();

    sqlWorkerThread<sqlWorkJobs> scheduler;

    scheduler.m_taskQueue.safe_push(sqlWorkJobs(sqlWorkJobs::taskType::swjStartJourney,testGuid));

    sleep(2);

    time_t rawtime;
    time(&rawtime);

    GstClockTime gtime=rawtime*GST_SECOND;
    //gtime+=(GST_SECOND/2);
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
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>


template <class item>
class lockableQueue 
{

protected:

    std::mutex m_lock;
    std::queue<item> m_queue;

public:

    item safe_pop()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        item popped=m_queue.front();
        m_queue.pop();
        return popped;
    }

    void safe_push(item theItem)
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_queue.push(theItem);
    }

    bool pending_jobs()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        bool ret=m_queue.size()>0;
        return ret;
    }

};


template <class item>
class taskScheduler
{
public:

    taskScheduler():
        m_taskQueueThread(staticEntry,this)
    {
    }

    virtual ~taskScheduler()
    {
        stop();
    }

    void stop()
    {
        if(m_taskRunning)
        {
            m_taskStop=true;
            m_taskQueueThread.join();
        }
    }

    static void staticEntry(void *data)
    {
        ((taskScheduler*)data)->threadEntry();        
    }

    virtual void runtask(item poppedTask)
    {
        poppedTask.run();
    }

    void threadEntry()
    {
        m_taskRunning=true;
        m_taskStop=false;

        while(!m_taskStop || (m_taskStop && m_taskQueue.pending_jobs()))
        {
            if(m_taskQueue.pending_jobs())
            {
                item popped=m_taskQueue.safe_pop();
                runtask(popped);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        m_taskRunning=false;
    }

    lockableQueue<item> m_taskQueue;
    std::thread m_taskQueueThread;
    volatile bool m_taskStop, m_taskRunning;


};

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "dashcam_sql.h"

#include <gst/gst.h>

template <class item>
class sqlWorkerThread : public taskScheduler<item>
{

protected:

    mariaDBconnection m_sql;

public:
    sqlWorkerThread():
        m_sql("debian","dashcam","dashcam","dashcam")
    {

    }

    virtual void runtask(item poppedTask)
    {
        poppedTask.setSQLconnection(&m_sql);
        poppedTask.run();
    }

};

class task
{
public:
    virtual void run(){};
};


class sqlWorkJobs : public task
{
public:

    enum taskType {  swjStartJourney, 
                        swjUpdateJourney,
                        swjCreateChapter,
                        sqjCloseChapter,
                        swjEndJourney,
                        swjExpireJourneys
                        } m_tasktype;

protected:

    boost::uuids::uuid m_journeyid, m_chapterid;
    GstClockTime m_basetime;
    long long m_offsetms;
    std::string m_filepath;

    mariaDBconnection *m_sql;


public:
    // swjStartJourney
    // swjEndJourney
    sqlWorkJobs(taskType task, boost::uuids::uuid guid):
        m_tasktype(task),
        m_journeyid(guid),
        m_sql(NULL)
    {

    }       

    // swjUpdateJourney
    sqlWorkJobs(boost::uuids::uuid guid, GstClockTime basetime):
        m_tasktype(swjUpdateJourney),
        m_journeyid(guid),
        m_sql(NULL),
        m_basetime(basetime)
    {

    }       

    sqlWorkJobs(std::string filepath, 
                    boost::uuids::uuid journey_guid, 
                    boost::uuids::uuid chapter_guid, 
                    long long start_ms):
        m_tasktype(swjCreateChapter),
        m_journeyid(journey_guid),
        m_chapterid(chapter_guid),
        m_sql(NULL),
        m_offsetms(start_ms),
        m_filepath(filepath)
    {

    }       

    sqlWorkJobs(boost::uuids::uuid journey_guid, 
                    boost::uuids::uuid chapter_guid, 
                    long long end_ms):
        m_tasktype(sqjCloseChapter),
        m_journeyid(journey_guid),
        m_chapterid(chapter_guid),
        m_sql(NULL),
        m_offsetms(end_ms)
    {

    }

    sqlWorkJobs(GstClockTime basetime):
    m_tasktype(swjExpireJourneys),
    m_sql(NULL),
    m_basetime(basetime)
    {

    }       



/////////////////////////////////////////////////////

    void setSQLconnection(mariaDBconnection *sql)
    {
        m_sql=sql;
    }

    virtual void run()
    {
        switch(m_tasktype)
        {
            case swjStartJourney:
                startJourney();
                break;
            case swjUpdateJourney:
                updateJourney();
                break;
            case swjCreateChapter:
                addChapter();
                break;
            case sqjCloseChapter:
                closeChapter();
                break;
            case swjEndJourney:
                endJourney();
                break;
            case swjExpireJourneys:
                expireJourneys();
                break;
            default:
                break;
        }
    }


protected:


    void startJourney()
    {
        startEndJourney("CALL sp_new_journey(?,?)");
    }

    void endJourney()
    {
        startEndJourney("CALL sp_end_journey(?,?)");
    }

    void startEndJourney(const char*sql_sp)
    {
        mariaBinding<journey_params,2> startParams;
        startParams.m_id=boost::uuids::to_string(m_journeyid);
        mySQLprepared<journey_params,2> callCreateJourney(m_sql,sql_sp);

        if(callCreateJourney.bind(startParams))
        {
            if(!callCreateJourney.exec())
            {
                printf("SQL error: %s\r\n",m_sql->error());
            }
        }
        else
        {
            printf("SQL error: %s\r\n",m_sql->error());
        }
    }

    void updateJourney()
    {
        mariaBinding<journey_params,2> updateParams;
        updateParams.m_id=boost::uuids::to_string(m_journeyid);
        updateParams.m_when=maria_timestamp(m_basetime);
        mySQLprepared<journey_params,2> callUpdateJourney(m_sql,"CALL sp_set_journey_basetime(?,?)");

        if(callUpdateJourney.bind(updateParams))
        {
            if(!callUpdateJourney.exec())
            {
                printf("SQL error: %s\r\n",m_sql->error());
            }
        }
        else
        {
            printf("SQL error: %s\r\n",m_sql->error());
        }
    }


    void addChapter()
    {
        mariaBinding<chapter_params,4> updateParams;
        updateParams.m_journeyid=boost::uuids::to_string(m_journeyid);
        updateParams.m_chapterid=boost::uuids::to_string(m_chapterid);
        updateParams.m_filepath=m_filepath;
        updateParams.m_startms=m_offsetms;
        mySQLprepared<chapter_params,4> callUpdateJourney(m_sql,"CALL sp_create_journey_chapter(?,?,?,?)");

        if(callUpdateJourney.bind(updateParams))
        {
            if(!callUpdateJourney.exec())
            {
                printf("SQL error: %s\r\n",m_sql->error());
            }
        }
        else
        {
            printf("SQL error: %s\r\n",m_sql->error());
        }   
    }

    void closeChapter()
    {
        mariaBinding<close_chapter_params,3> updateParams;
        updateParams.m_journeyid=boost::uuids::to_string(m_journeyid);
        updateParams.m_chapterid=boost::uuids::to_string(m_chapterid);
        updateParams.m_endms=m_offsetms;
        mySQLprepared<close_chapter_params,3> callUpdateJourney(m_sql,"CALL sp_close_journey_chapter(?,?,?)");

        if(callUpdateJourney.bind(updateParams))
        {
            if(!callUpdateJourney.exec())
            {
                printf("SQL error: %s\r\n",m_sql->error());
            }
        }
        else
        {
            printf("SQL error: %s\r\n",m_sql->error());
        }           
    }

    void expireJourneys()
    {
        mariaBinding<expire_journeys_params,7> expireParams;
        expireParams.m_cutoff=maria_timestamp(m_basetime);
        mySQLprepared<expire_journeys_params,7> callExpireJourneys(m_sql,"CALL sp_list_old_chapters(?)");

        std::vector<std::pair<std::string, std::pair<maria_guid,maria_guid>>> killthemall;

        //mariaBinding<chapter_view_cols,7> expiredChapters;
        if(callExpireJourneys.bind(expireParams))
        {

            if(callExpireJourneys.execAndFetch(expireParams,[&](int flags)
                {
                    //printf("%s\n\r",expireParams.m_filename);
                    killthemall.push_back(std::pair<std::string, std::pair<maria_guid,maria_guid>>(   expireParams.m_filename,
                                                                                                      std::pair<maria_guid,maria_guid>(expireParams.m_journeyid,expireParams.m_chapterid)));
                }))
            {
                mariaBinding<delete_chapter_params,2> deleteParams;

                for(auto iter=killthemall.begin();iter!=killthemall.end();iter++)
                {
                    // remove the file
                    unlink(iter->first.c_str());
                    // update the db
                    deleteParams.m_journeyid=iter->second.first;
                    deleteParams.m_chapterid=iter->second.second;
                    mySQLprepared<delete_chapter_params,2> callExpireJourneys(m_sql,"CALL sp_delete_chapter(?,?)");

                    if(callExpireJourneys.bind(deleteParams))
                    {
                        if(!callExpireJourneys.exec())
                        {
                            printf("SQL error: %s\r\n",m_sql->error());
                        }
                    }
                    else
                    {
                        printf("SQL error: %s\r\n",m_sql->error());
                    }           

                }
            }
            else
            {
                printf("SQL error: %s\r\n",m_sql->error());
            }           
        }
        else
        {
            printf("SQL error: %s\r\n",m_sql->error());
        }           

    }

};
#ifndef _dashcam_sql_h
#define _dashcam_sql_h

#include "maria_queries.h"
#include <gst/gst.h>

// some helpers 

class maria_timestamp
{
public:

    maria_timestamp(GstClockTime ct)
    {
        // work it out from epoch
        time_t epochOffset=ct/GST_SECOND;
        // epochOffset is seconds of granularity
        struct tm *now = gmtime(&epochOffset);
        // work out the diff
        ct-=epochOffset*GST_SECOND;
        // and turn to millis 
        setMyTime(now, ct/1000000);


    }

    maria_timestamp()
    {
        setMyTime();
    }

    void setMyTime(struct tm *then=NULL, unsigned long ms=0)
    {
        if(!then)
        {
            time_t rawtime;
            struct tm *info;
            time(&rawtime);
            then = gmtime(&rawtime);
        }


        m_now.year=then->tm_year+1900;
        m_now.month=then->tm_mon+1;
        m_now.day=then->tm_mday;
        m_now.hour=then->tm_hour;
        m_now.minute=then->tm_min;
        m_now.second=then->tm_sec;
        m_now.second_part=ms*1000;
        m_now.neg=false;
        m_now.time_type=MYSQL_TIMESTAMP_DATETIME;

    }

    void operator =(const maria_timestamp &other)
    {
        memcpy(&m_now,&other.m_now,sizeof(m_now));
    }

    MYSQL_TIME m_now;

};

class maria_guid
{
public:

    maria_guid()
    {
        memset(m_guid,0,size());
    }

    bool operator =(const std::string &other)
    {
        if(other.size()!=size())
        {
            return false;
        }

        strncpy(m_guid,other.c_str(),size());

        return true;
    }

    operator void*() { return &m_guid; }
    size_t size() { return sizeof(m_guid)-1; }

protected:

    char m_guid[37];
};


// journey entry

class journey_params : public nullAccessor
{
public:

    journey_params()
    {

    }

    // the params for the query 
    maria_timestamp m_when;
    maria_guid m_id;

    virtual void fillBind(size_t,MYSQL_BIND *bind, int *dir)
    {
        // fill the 'in' params
        bind[0].buffer_type=MYSQL_TYPE_DATETIME;
        bind[0].buffer=&m_when.m_now;
        bind[0].buffer_length=sizeof(MYSQL_TYPE_DATETIME);
        dir[0]=SQL_PARAM_IN;

        bind[1].buffer_type=MYSQL_TYPE_STRING;
        bind[1].buffer=m_id;
        bind[1].buffer_length=m_id.size();
        dir[1]=SQL_PARAM_IN;

    }

    // virtual void fillBindOut(size_t,MYSQL_BIND *bind, int *dir){}
    // virtual void fillBindRowset(size_t,MYSQL_BIND *bind, int *dir){}

};

class chapter_params : public nullAccessor
{
public:

    maria_guid m_journeyid, m_chapterid;
    long long m_startms;
    std::string m_filepath;

    virtual void fillBind(size_t,MYSQL_BIND *bind, int *dir)
    {
        // fill the 'in' params
        bind[0].buffer_type=MYSQL_TYPE_LONGLONG;
        bind[0].buffer=&m_startms;
        bind[0].buffer_length=sizeof(m_startms);
        dir[0]=SQL_PARAM_IN;

        bind[1].buffer_type=MYSQL_TYPE_STRING;
        bind[1].buffer=m_journeyid;
        bind[1].buffer_length=m_journeyid.size();
        dir[1]=SQL_PARAM_IN;

        bind[2].buffer_type=MYSQL_TYPE_STRING;
        bind[2].buffer=m_chapterid;
        bind[2].buffer_length=m_chapterid.size();
        dir[2]=SQL_PARAM_IN;        

        bind[3].buffer_type=MYSQL_TYPE_STRING;
        bind[3].buffer=(void*)m_filepath.c_str();
        bind[3].buffer_length=m_filepath.size();
        dir[3]=SQL_PARAM_IN;

    }


};

class delete_chapter_params : public nullAccessor
{
public:

    maria_guid m_journeyid, m_chapterid;

    virtual void fillBind(size_t,MYSQL_BIND *bind, int *dir)
    {
        bind[0].buffer_type=MYSQL_TYPE_STRING;
        bind[0].buffer=m_journeyid;
        bind[0].buffer_length=m_journeyid.size();
        dir[0]=SQL_PARAM_IN;

        bind[1].buffer_type=MYSQL_TYPE_STRING;
        bind[1].buffer=m_chapterid;
        bind[1].buffer_length=m_chapterid.size();
        dir[1]=SQL_PARAM_IN;        


    }
};

class close_chapter_params : public nullAccessor
{
public:

    maria_guid m_journeyid, m_chapterid;
    long long m_endms;

    virtual void fillBind(size_t,MYSQL_BIND *bind, int *dir)
    {
        // fill the 'in' params
        bind[0].buffer_type=MYSQL_TYPE_LONGLONG;
        bind[0].buffer=&m_endms;
        bind[0].buffer_length=sizeof(m_endms);
        dir[0]=SQL_PARAM_IN;

        bind[1].buffer_type=MYSQL_TYPE_STRING;
        bind[1].buffer=m_journeyid;
        bind[1].buffer_length=m_journeyid.size();
        dir[1]=SQL_PARAM_IN;

        bind[2].buffer_type=MYSQL_TYPE_STRING;
        bind[2].buffer=m_chapterid;
        bind[2].buffer_length=m_chapterid.size();
        dir[2]=SQL_PARAM_IN;        


    }


};

class chapterViewAccessor : public nullAccessor
{
public:

    maria_guid m_journeyid, m_chapterid;
    maria_timestamp m_cutoff;
    maria_timestamp m_journeyStart, m_journeyEnd, m_chapterStart, m_chapterEnd;
    char m_filename[1024];
    //unsigned char m_locked;

    virtual void fillBindRowset(size_t,MYSQL_BIND *bind, int *dir)
    {

        bind[0].buffer_type=MYSQL_TYPE_STRING;
        bind[0].buffer=m_journeyid;
        bind[0].buffer_length=m_journeyid.size();

        bind[1].buffer_type=MYSQL_TYPE_STRING;
        bind[1].buffer=m_chapterid;
        bind[1].buffer_length=m_chapterid.size();

        bind[2].buffer_type=MYSQL_TYPE_DATETIME;
        bind[2].buffer=&m_journeyStart.m_now;
        bind[2].buffer_length=sizeof(MYSQL_TYPE_DATETIME);

        bind[3].buffer_type=MYSQL_TYPE_DATETIME;
        bind[3].buffer=&m_journeyEnd.m_now;
        bind[3].buffer_length=sizeof(MYSQL_TYPE_DATETIME);

        bind[4].buffer_type=MYSQL_TYPE_DATETIME;
        bind[4].buffer=&m_chapterStart.m_now;
        bind[4].buffer_length=sizeof(MYSQL_TYPE_DATETIME);

        bind[5].buffer_type=MYSQL_TYPE_DATETIME;
        bind[5].buffer=&m_chapterEnd.m_now;
        bind[5].buffer_length=sizeof(MYSQL_TYPE_DATETIME);

        bind[6].buffer_type=MYSQL_TYPE_STRING;
        bind[6].buffer=&m_filename;
        bind[6].buffer_length=sizeof(m_filename)-1;

        // bind[7].buffer_type=MYSQL_TYPE_TINY;
        // bind[7].buffer=&m_locked;
        // bind[7].buffer_length=sizeof(m_locked);

    }


};


class expire_journeys_params : public chapterViewAccessor
{
public:

    virtual void fillBind(size_t,MYSQL_BIND *bind, int *dir)
    {
        bind[0].buffer_type=MYSQL_TYPE_DATETIME;
        bind[0].buffer=&m_cutoff.m_now;
        bind[0].buffer_length=sizeof(MYSQL_TYPE_DATETIME);
        dir[0]=SQL_PARAM_IN;
    }
};

#endif
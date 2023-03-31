#ifndef _mysqlhelpers_guard
#define _mysqlhelpers_guard
#include "maria_database.h"

#include <array>
#include <string>


#define SQL_PARAM_IN    1
#define SQL_PARAM_OUT   2


// accessor interface essentially
class nullAccessor
{

protected:

    virtual void fillBind(size_t,MYSQL_BIND *bind, int *dir){}
    virtual void fillBindOut(size_t,MYSQL_BIND *bind, int *dir){}
    virtual void fillBindRowset(size_t,MYSQL_BIND *bind, int *dir){}

};

// sits on top of data as an accessor
template <class access,size_t count=10>
class mariaBinding : public access
{
protected:

    std::array<MYSQL_BIND,count> m_bindings;
    std::array<int,count> m_directions;

public:
    mariaBinding()
    {
        // clean the BINDs 
        MYSQL_BIND *bindings=(MYSQL_BIND*)m_bindings.data();
        for(auto each=0;each<count;each++)
        {
            memset(&bindings[each],0,sizeof(MYSQL_BIND));
        }
    }

    ~mariaBinding()
    {
    }

    operator MYSQL_BIND *() { return m_bindings.data(); }

    void Prepare()
    {
        access::fillBind(count,m_bindings.data(),m_directions.data());
    }

    void PrepareOut(MYSQL_FIELD*)
    {
        access::fillBindOut(count,m_bindings.data(),m_directions.data());
    }

    void PrepareRowset(MYSQL_FIELD*)
    {
        access::fillBindRowset(count,m_bindings.data(),m_directions.data());
    }


};

// https://dev.mysql.com/doc/c-api/8.0/en/c-api-prepared-call-statements.html

// a prepared statement - 
template <class access,size_t bindcount>
class mySQLprepared
{
public:
    mySQLprepared(mariaDBconnection *conn, std::string statement):
        m_statement(NULL),
        m_conn(conn)
    {
         // spin up
         m_statement=mysql_stmt_init(*m_conn);

         // prep
         mysql_stmt_prepare(m_statement,statement.c_str(),statement.length());
        
    }

    ~mySQLprepared()
    {
        close();
    }

    bool bind(mariaBinding<access,bindcount> &bind)
    {
        if(!m_conn || !m_statement)
            return false;
        
        bind.Prepare();

        return mysql_stmt_bind_param(m_statement,bind)==0;
    }

    bool exec()
    {
        if(m_statement)
        {
            return mysql_stmt_execute(m_statement)==0;
        }

        return false;
    }

    // fetch and foreach call the lambda
    bool fetch(mariaBinding<access,bindcount> &bind, const std::function <void (int)>& readRowset=[](int){})
    {

        int status=0;
        do 
        {
            unsigned num_fields = mysql_stmt_field_count(m_statement);


            if(num_fields)
            {
                MYSQL_RES *rs_metadata = mysql_stmt_result_metadata(m_statement);
                MYSQL_FIELD * fields = mysql_fetch_fields(rs_metadata);

                if(m_conn->status() & SERVER_PS_OUT_PARAMS)
                {
                    bind.PrepareOut(fields);
                }
                else
                {
                    bind.PrepareRowset(fields);
                }

                unsigned status = mysql_stmt_bind_result(m_statement, bind);

                if(status)
                {
                    return false;
                }

                for(status=mysql_stmt_fetch(m_statement);
                    status == 1 || status == MYSQL_NO_DATA;
                    status=mysql_stmt_fetch(m_statement))
                {
                    readRowset(m_conn->status());
                }

                mysql_free_result(rs_metadata); 

            }

        status=mysql_stmt_next_result(m_statement);
        } while(!status);

        return true;
    }

    void close()
    {
        if(m_statement)
            mysql_stmt_close(m_statement);
            
        m_statement=NULL;
    }

    // `kitchen sink` call
    bool execAndFetch(mariaBinding<access,bindcount> &bind, const std::function <void (int)>& readRowset=[](int){})
    {
        if(!exec())
        {
            return false;
        }

        bool ret=fetch(bind,readRowset);

        close();

        return ret;
    }

    const char *error() { return mysql_stmt_error(m_statement); }

protected:

    MYSQL_STMT *m_statement;
    mariaDBconnection* m_conn;
};

#endif // _mysqlhelpers_guard
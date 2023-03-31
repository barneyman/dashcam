#ifndef __maria_db_guard    
#define __maria_db_guard

#include "mysql.h"
#include "gstreamHelpers/json/json.hpp"

class mariaDBconnection
{
public:

    mariaDBconnection(nlohmann::json &jsonConfig):
        m_con(NULL)
    {
        if(jsonConfig.contains("SQL"))
        {
            nlohmann::json sqlConfig=jsonConfig["SQL"];

            lateCtor(sqlConfig["server"].get<std::string>().c_str(),
                sqlConfig["user"].get<std::string>().c_str(),
                sqlConfig["pwd"].get<std::string>().c_str(),
                sqlConfig["schema"].get<std::string>().c_str(),
                sqlConfig["port"].get<int>()
                );            
        }
    }


    mariaDBconnection(const char* server, const char *user, const char* pwd, const char * schema, unsigned port=3306):
        m_con(NULL)
    {
        lateCtor(server,user,pwd,schema,port);
    }

    ~mariaDBconnection()
    {
        closedb();
    }

    operator MYSQL* () { return m_con; }

    bool valid() { return m_con!=NULL; }

    const char* error() { return mysql_error(*this); }

    unsigned status() { return m_con?m_con->server_status:0; }

protected:

    void closedb()
    {
        if(m_con)
        {
            mysql_close(m_con);
            m_con=NULL;
        }
    }

    void lateCtor(const char* server, const char *user, const char* pwd, const char * schema, unsigned port=3306)
    {
        m_con= mysql_init(NULL);

        if (m_con == NULL) 
        {
            fprintf(stderr, "%s\n", mysql_error(m_con));
            return;
        }

        if (mysql_real_connect(m_con, server, user, pwd, NULL, port, NULL, CLIENT_PS_MULTI_RESULTS) == NULL) 
        {
            fprintf(stderr, "%s\n", mysql_error(m_con));
            closedb();
            return;
        }  

        if(mysql_select_db(m_con,schema)) 
        {
            fprintf(stderr, "%s\n", mysql_error(m_con));
            closedb();
            return;
        }
    }


    MYSQL *m_con;

};

#endif // __maria_db_guard

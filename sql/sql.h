#include <iostream> 
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept> 
#include "mysql_connection.h" 
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace std;

class Sql{
    public:
        Sql(string host,string user, string password, string database);
        ~Sql();

        bool connect();
        bool query(string query);
        
        sql::ResultSet *res=nullptr;

    private:
        string host;string user; string password; string database;

        sql::Driver *driver=nullptr;
        sql::Connection *con=nullptr; 

};
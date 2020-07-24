#include "sql.h" 
#include <string>
#include <algorithm>

using namespace std;
Sql::Sql(string host,string user, string password, string database){
    this->host = host;
    this->user = user;
    this->password = password;
    this->database = database;
}

Sql::~Sql(){
    cout<<"SQL Destroyed!\n"; 
    if(res != nullptr) {delete res;res=nullptr;} 
    if(con != nullptr) {delete con;con=nullptr;} 
}

bool Sql::connect(){
    try{ 
        if(con != nullptr) {delete con;con=nullptr;} 
        driver = get_driver_instance();
        con = driver->connect(host, user, password); 
        con->setSchema(database);
        return true;
    } catch (sql::SQLException &e) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line "<< __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
        return false;
    }
}

bool Sql::query(string query){
    sql::Statement *stmt=nullptr; 
    try{ 
        if(res != nullptr) {delete res;res=nullptr;}  
        stmt = con->createStatement();

        string str = query;
        transform(str.begin(), str.end(), str.begin(), ::toupper);
        size_t found = str.find("SELECT");
        if(found!=string::npos){ 
            res = stmt->executeQuery(query);
        }else{
            stmt->execute(query);
        }
        if(stmt != nullptr) {delete stmt;stmt=nullptr;} 
        return true;
    }catch (sql::SQLException &e) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line "<< __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl; 
        if(stmt != nullptr) {delete stmt;stmt=nullptr;} 
        return false;
    }

}
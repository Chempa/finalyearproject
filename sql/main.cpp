#include <iostream>
#include "sql.h"

using namespace std;

string host = "localhost";
string user = "pharmacy_admin";
string password = "admin";
string database = "pharmacy_db";

int main(){
    Sql sql(host, user, password, database);
    if(sql.connect()){
        cout<<"Connected Successfully\n";
    }else{
        cout<<"Connection failed\n";
    }

    if(sql.query("UPDATE products SET product='COVID-19 Cure' where product_id=1;")){
        cout<<"Query success\n";
    }else{
        cout<<"Query failed\n";
    }

    

    return 0;
}
#include <string>
#include <pqxx/pqxx>
#include <iostream>
#include "postgresql.h"
#include "tables.cpp"
namespace psql
{
    // connect_string "dbname=your_db user=your_user password=your_password hostaddr=your_server port=5432"
    DB::DB(const std::string &connect_info)
    {
        c_info = connect_info;
    };

};
// Verify Tables
void check_tables(std::string &c)
{
    try
    {
        pqxx::connection conn(c);
        std::cout << "[II] Connected to DB" << std::endl;
        std::cout << "[II] Verifying DB scheme" << std::endl;
        pqxx::work tr{conn};
        if(verify::user(tr)){
            std::cout << "[II] User tables exists" << std::endl;
        }
        else{
            create_tables::create_users(tr);
            std::cout << "[EE] Cannot find user table, creating new one" << std::endl;
        }
        if(verify::wch_table(tr)){
            std::cout << "[II] User tables exists" << std::endl;
        }
        else{
            create_tables::create_wch(tr);
            std::cout << "[EE] Cannot find watchers table, creating new one" << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}
#include <string>
#include <pqxx/pqxx>
#include <iostream>
#include "postgresql.h"
#include "tables.cpp"
#include <fstream>
#include <chrono>
#include <thread>
#include "../json.hpp"
void check_tables(std::string &c);
using json = nlohmann::json;
pqxx::connection read_conn;
namespace psql
{
    // Init DB
    DB::DB()
    {
        std::ifstream config("config.json");
        json conf = json::parse(config);
        config.close();
        c_info = "dbname=" + conf["db"]["dbname"].get<std::string>() +
                 " user=" + conf["db"]["user"].get<std::string>() +
                 " password=" + conf["db"]["password"].get<std::string>() +
                 " port=" + conf["db"]["port"].get<std::string>();
        check_tables(c_info);
    }
    // INIT нового пользователя
    void DB::new_user(int64_t &id, std::string &name, std::string &username)
    {
        try
        {
            pqxx::connection conn(c_info);
            std::cout << "[II] Adding new user to DB (" << name << " " << username << ")" << std::endl;
            pqxx::work tr(conn);
            tr.exec("INSERT INTO users (id) VALUES ('" + std::to_string(id) + "') ON CONFLICT(chatid) DO NOTHING;");
            tr.exec("UPDATE users SET Username = '" + username + "' WHERE id ='" + std::to_string(id) + "';UPDATE users SET Name = '" + name + "' WHERE id ='" + std::to_string(id) + "';");
            tr.exec("UPDATE users SET isAutosend = 'FALSE' WHERE id ='" + std::to_string(id) + "';");
            tr.exec("UPDATE users SET isAdmin = 'FALSE' WHERE id ='" + std::to_string(id) + "';");
            tr.commit();
            conn.close();
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
        }
    }
    void DB::init_read_connection()
    {
        while (!read_conn.is_open())
        {
            try
            {
                read_conn = pqxx::connection(c_info);
            }
            catch (std::exception &e)
            {
                std::cout << "[EE] Error creating reading connection: " << e.what() << std::endl;
                std::cout << "[EE] Retrying in 5 secconds" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    bool DB::check_admin(int64_t &id)
    {
        try
        {
            pqxx::work tr{read_conn};
            bool result = tr.query_value<bool>("SELECT isAdmin FROM users WHERE id = " + std::to_string(id));
            tr.commit();
            return result;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error getting data from DB: " << e.what() << std::endl;
        }
    }

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
        if (verify::user(tr))
        {
            std::cout << "[II] User tables exists" << std::endl;
        }
        else
        {
            create_tables::create_users(tr);
            std::cout << "[EE] Cannot find user table, creating new one" << std::endl;
        }
        if (verify::wch_table(tr))
        {
            std::cout << "[II] User tables exists" << std::endl;
        }
        else
        {
            create_tables::create_wch(tr);
            std::cout << "[EE] Cannot find watchers table, creating new one" << std::endl;
        }
        if (verify::dates(tr))
        {
            std::cout << "[II] Dates tables exists" << std::endl;
        }
        else
        {
            create_tables::create_dates(tr);
            std::cout << "[EE] Cannot find dates table, creating new one" << std::endl;
        }
        tr.commit();
        conn.close();
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] " << e.what() << std::endl;
    }
}
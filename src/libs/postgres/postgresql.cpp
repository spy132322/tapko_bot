#include <string>
#include <pqxx/pqxx>
#include <iostream>
#include "postgresql.h"
#include "tables.cpp"
#include <fstream>
#include <chrono>
#include <thread>
#include "../json.hpp"
#include <vector>
bool stop_s = false;

void check_tables(std::string &c);
int get_aval_id();
bool check_if_exists(std::string name);

using json = nlohmann::json;
// Read connection to commands
pqxx::connection read_conn;
// Write connection to commands
pqxx::connection write_conn;
// Read // Write connection to automatics
pqxx::connection auto_conn;
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
                 " port=" + conf["db"]["port"].get<std::string>()+
                 " host=" + conf["db"]["host"].get<std::string>();
        check_tables(c_info);
    }
    // INIT нового пользователя
    void DB::new_user(int64_t &id, std::string &name, std::string &username)
    {

        std::cout << "[II] Adding new user to DB (" << name << " " << username << ")" << std::endl;
        pqxx::work tr(write_conn);
        try
        {
            tr.exec("INSERT INTO users (id) VALUES ('" + std::to_string(id) + "') ON CONFLICT(chatid) DO NOTHING;");
            tr.exec("UPDATE users SET Username = '" + username + "' WHERE id ='" + std::to_string(id) + "';UPDATE users SET Name = '" + name + "' WHERE id ='" + std::to_string(id) + "';");
            tr.exec("UPDATE users SET isAutosend = 'FALSE' WHERE id ='" + std::to_string(id) + "';");
            tr.exec("UPDATE users SET isAdmin = 'FALSE' WHERE id ='" + std::to_string(id) + "';");
            tr.commit();
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error adding user to db " << e.what() << std::endl;
            tr.abort();
        }
    }
    void DB::connection_watchdog()
    {
        std::cout << "[II] Connection watchdog is up" << std::endl;
        while (!stop_s)
        {
            std::cout << read_conn.is_open() << std::endl;
            std::cout << write_conn.is_open() << std::endl;
            std::cout << auto_conn.is_open() << std::endl;
            // Соеденение для чтения комманд пользователей
            while (!read_conn.is_open())
            {
                try
                {   
                    std::cout << c_info <<std::endl;
                    read_conn = pqxx::connection(c_info);
                }
                catch (std::exception &e)
                {
                    std::cout << "[EE] Error creating reading connection: " << e.what() << std::endl;
                    std::cout << "[EE] Retrying in 5 secconds" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
            // Соеденение для записи комманд пользователей
            while (!write_conn.is_open())
            {
                try
                {
                    write_conn = pqxx::connection(c_info);
                    if (write_conn.is_open())
                    {
                        std::cout << "[II] Write connection is ready" << std::endl;
                    }
                }
                catch (pqxx::broken_connection &e)
                {
                    std::cout << "[EE] Error creating write connection: " << e.what() << std::endl;
                    std::cout << "[EE] Retrying in 5 secconds" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
            // Соеденение с бд для автоматики
            while (!auto_conn.is_open())
            {
                try
                {
                    auto_conn = pqxx::connection(c_info);
                    if (auto_conn.is_open())
                    {
                        std::cout << "[II] Automatics connection is ready" << std::endl;
                    }
                }
                catch (pqxx::broken_connection &e)
                {
                    std::cout << "[EE] Error creating automatics connection: " << e.what() << std::endl;
                    std::cout << "[EE] Retrying in 5 secconds" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    // Проверка на админа user тг
    bool DB::check_admin(int64_t &id)
    {
        pqxx::work tr{read_conn};
        try
        {
            bool result = tr.query_value<bool>("SELECT isAdmin FROM users WHERE id = " + std::to_string(id));
            tr.commit();
            return result;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error getting data from DB: " << e.what() << std::endl;
            tr.abort();
            return false;
        }
    }
    // Дабовлялка в бд дежурного
    int DB::add(std::string name)
    {
        pqxx::work tr{write_conn};
        try
        {
            if (!check_if_exists)
            {
                int aval_id = get_aval_id();
                tr.exec("INSERT INTO watchers (id, Name, isKilled, isWas) VALUES (" + std::to_string(aval_id) + "'" + name + "', FALSE, FALSE);");
                tr.commit();
                return 0;
            }
            else
            {
                tr.commit();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error adding watcher: " << e.what() << std::endl;
            tr.abort();
            return 2;
        }
    }
    // Читаем всех дежурных
    std::vector<DB::GuyData> DB::list(){
        pqxx::work tr{read_conn};
        std::vector<DB::GuyData> result;
        for(auto& [id, Name, isKilled, isWas] : tr.query<int, std::string, bool, bool>("")){
            result[id].id = id;
            result[id].Name = Name;
            result[id].isKilled = isKilled;
            result[id].isWas = isWas;
        }
        return result;
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
    catch (pqxx::broken_connection &e)
    {
        std::cout << "[EE] " << e.what() << std::endl;
    }
}
// Поиск доступного id наблюдателя
bool check_if_exists(std::string name)
{

    pqxx::work tr{auto_conn};
    try
    {
        bool result = tr.query_value<bool>("SELECT EXISTS (SELECT 1 FROM watchers WHERE name = '" + name + "') AS name_exists;");
        tr.commit();
        return result;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Unable to get existment status of watcher" << std::endl;
        tr.abort();
        return true;
    }
}
// Поиск доступного id
int get_aval_id()
{
    pqxx::work tr{auto_conn};
    try
    {
        int id = tr.query_value<int>(R"(
WITH RECURSIVE available_id AS (
    SELECT 0 AS id -- Начинаем с 0
    UNION ALL
    SELECT id + 1
    FROM available_id
    WHERE id + 1 NOT IN (SELECT id FROM users)
    AND id < (SELECT MAX(id) FROM users) -- Ограничиваем максимальным id
)
SELECT id FROM available_id
ORDER BY id
LIMIT 1;
)");
        tr.commit();
        return id;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Getting max id of watcher error:  " << e.what() << std::endl;
        tr.abort();
        return 10000;
    }
}
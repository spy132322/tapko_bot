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
#include <memory>
bool stop_s = false;

void check_tables(std::string &c);
int get_aval_id(pqxx::connection &conn);
bool check_if_exists(std::string name, pqxx::connection &conn);
bool check_user_if_exists(int64_t &id, pqxx::connection &c);
using json = nlohmann::json;

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
                 " port=" + conf["db"]["port"].get<std::string>() +
                 " host=" + conf["db"]["host"].get<std::string>();
        check_tables(c_info);
    }
    // INIT нового пользователя
    void DB::new_user(int64_t &id, std::string &name, std::string &username)
    {
        try
        {
            pqxx::connection conn(c_info);
            if (!check_user_if_exists(id, conn))
            {
                std::cout << "[II] Adding new user to DB (" << name << " " << username << ")" << std::endl;
                pqxx::work tr{conn};

                tr.exec("INSERT INTO users (id) VALUES ('" + std::to_string(id) + "') ON CONFLICT(id) DO NOTHING;");
                tr.exec("UPDATE users SET Username = '" + username + "' WHERE id ='" + std::to_string(id) + "';UPDATE users SET Name = '" + name + "' WHERE id ='" + std::to_string(id) + "';");
                tr.exec("UPDATE users SET isAutosend = 'FALSE' WHERE id ='" + std::to_string(id) + "';");
                tr.exec("UPDATE users SET isAdmin = 'FALSE' WHERE id ='" + std::to_string(id) + "';");
                tr.commit();
            }
            conn.close();
        }
        catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    // Проверка на админа user тг
    bool DB::check_admin(int64_t &id)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
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
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            return false;
        }
    }
    // Дабовлялка в бд дежурного
    int DB::add(std::string name)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            try
            {
                if (!check_if_exists(name, conn))
                {
                    int aval_id = get_aval_id(conn);
                    tr.exec("INSERT INTO watchers (id, Name, isKilled, isWas) VALUES (" + std::to_string(aval_id) + "'" + name + "', FALSE, FALSE);");
                    tr.commit();
                    conn.close();
                    return 0;
                }
                else
                {
                    tr.commit();
                    conn.close();
                    return 1;
                }
            }
            catch (std::exception &e)
            {
                std::cout << "[EE] Error adding watcher: " << e.what() << std::endl;
                tr.abort();
                conn.close();
                return 2;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            return 2;
        }
    }
    // Читаем всех дежурных
    std::vector<DB::GuyData> DB::list()
    {
        std::vector<DB::GuyData> result;
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};

            for (auto &[id, Name, isKilled, isWas] : tr.query<int, std::string, bool, bool>("SELECT * FROM watchers;"))
            {
                result[id].id = id;
                result[id].Name = Name;
                result[id].isKilled = isKilled;
                result[id].isWas = isWas;
            }
            tr.commit();
            conn.close();
            return result;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            result[0].id = 0;
            result[0].Name = "[EE] Error reading DB";
            result[0].isKilled = true;
            result[0].isWas = true;
            return result;
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
            std::cout << "[II] Watchers tables exists" << std::endl;
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
// Поиск доступного id наблюдателя
bool check_if_exists(std::string name, pqxx::connection &conn)
{
    pqxx::work tr{conn};
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
bool check_user_if_exists(int64_t &id, pqxx::connection &c)
{
    pqxx::work tr{c};
    try
    {

        bool result = tr.query_value<bool>("SELECT EXISTS (SELECT 1 FROM users WHERE id = '" + std::to_string(id) + "') AS name_exists;");

        tr.commit();
        return result;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Unable to get existment status of user" << std::endl;
        tr.abort();
        return true;
    }
}
// Поиск доступного id
int get_aval_id(pqxx::connection &conn)
{
    pqxx::work tr{conn};
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
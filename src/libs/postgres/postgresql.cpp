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
#include "SQL.h"

void check_tables(std::string &c);
int get_aval_id(pqxx::work &tr);
bool check_if_exists(std::string name, pqxx::work &tr);
bool check_if_exists(int id, pqxx::work &tr);
bool check_user_if_exists(int64_t &id, pqxx::connection &c);
bool check_if_date_exists(std::string date, pqxx::work &tr);

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
                if (!check_if_exists(name, tr))
                {

                    int aval_id = get_aval_id(tr);
                    if (aval_id != -1)
                    {
                        tr.exec("INSERT INTO watchers (id, Name, isKilled, isWas) VALUES ('" + std::to_string(aval_id) + "','" + name + "', FALSE, FALSE);");
                        tr.commit();
                        conn.close();
                        return 0;
                    }
                    else
                    {
                        conn.close();
                        return 2;
                    }
                }
                else
                {
                    tr.abort();
                    conn.close();
                    return 1;
                }
            }
            catch (std::exception &e)
            {
                std::cout << "[EE] Error adding watcher: " << e.what() << std::endl;
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
    // Удалить Дежурного из БД
    int DB::del(int id)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            try
            {
                if (check_if_exists(id, tr))
                {
                    tr.exec("DELETE FROM watchers WHERE id = " + std::to_string(id) + ";");
                    tr.commit();
                    conn.close();
                    return 0;
                }
                else
                {
                    tr.abort();
                    conn.close();
                    return 1;
                }
            }
            catch (std::exception &e)
            {
                std::cout << "[EE] Error adding watcher: " << e.what() << std::endl;
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
        // List all

        std::vector<DB::GuyData> result;
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            if (tr.query_value<bool>(SQL::verify_exists_1s_tables_unit))
            {
                for (auto &[id, Name, isKilled, isWas] : tr.query<int, std::string, bool, bool>(SQL::get_all_watchers))
                {
                    result.push_back({id, Name, isKilled, isWas});
                }
            }
            else
            {
                result.push_back({0, "Список пуст", true, true});
            }

            tr.commit();
            conn.close();
            return result;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            result.push_back({0, "[EE] Error reading DB", true, true});
            return result;
        }
    }
    // Добавить дату в БД
    int DB::add_date(std::string date)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::transaction tr(conn);
            if (!check_if_date_exists(date, tr))
            {
                tr.exec("INSERT INTO dates (date) VALUES ('" + date + "');");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                tr.abort();
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error adding date to DB " << e.what() << std::endl;
            return 2;
        }
    }
    // Получить все исключенный даты
    std::vector<std::string> DB::list_dates()
    {
        std::vector<std::string> result;
        try
        {
            std::vector<std::string> result;
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            for (auto &[date] : tr.query<std::string>(SQL::get_all_dates))
            {
                result.push_back(date);
            }
            if (result.empty())
            {
                result.push_back("LIE");
                tr.commit();
                conn.close();
                return result;
            }
            else
            {
                tr.commit();
                conn.close();
                return result;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error getting list of dates " << e.what() << std::endl;
            result.push_back("EE");
            return result;
        }
    }
    // Удалить Дату из бд
    int DB::del_date(std::string date)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::transaction tr(conn);
            if (check_if_date_exists(date, tr))
            {
                tr.exec("DELETE FROM dates WHERE date='" + date + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                tr.abort();
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error deleting date to DB " << e.what() << std::endl;
            return 2;
        }
    }
    // Set watcher WAS to TRUE
    int DB::SetWas(int id)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            if (check_if_exists(id, tr))
            {
                tr.exec("UPDATE watchers SET isWas = TRUE WHERE id='" + std::to_string(id) + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                tr.abort();
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error when updating watcher " << e.what() << std::endl;
            return 2;
        }
    }
    // Set watcher WAS to FALSE
    int DB::UnSetWas(int id)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            if (check_if_exists(id, tr))
            {
                tr.exec("UPDATE watchers SET isWas = FALSE WHERE id='" + std::to_string(id) + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                tr.abort();
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error when updating watcher " << e.what() << std::endl;
            return 2;
        }
    }
    // Set watcher kill to TRUE
    int DB::Kill(int id)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            if (check_if_exists(id, tr))
            {
                tr.exec("UPDATE watchers SET isKilled = TRUE WHERE id='" + std::to_string(id) + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                tr.abort();
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error when updating watcher " << e.what() << std::endl;
            return 2;
        }
    }
    // Set watcher kill to FALSE
    int DB::unKill(int id)
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            if (check_if_exists(id, tr))
            {
                tr.exec("UPDATE watchers SET isKilled = FALSE WHERE id='" + std::to_string(id) + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                tr.abort();
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error when updating watcher " << e.what() << std::endl;
            return 2;
        }
    }
    // Set all watchers Was to FALSE
    void DB::clearall()
    {
        try
        {
            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            tr.exec("UPDATE watchers SET isWas = FALSE WHERE isWas=TRUE;");
            tr.commit();
            conn.close();
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error reseting watchers " << e.what() << std::endl;
        }
    }
    // list users with enables autosend
    std::vector<int64_t> DB::list_users()
    {

        std::vector<int64_t> result;
        try
        {

            pqxx::connection conn(c_info);
            pqxx::work tr{conn};
            for (auto &[ids] : tr.query<int>(SQL::get_all_users))
            {
                result.push_back(ids);
            }
            if (result.empty())
            {
                result.clear();
                tr.commit();
                conn.close();
                return result;
            }
            else
            {
                tr.commit();
                conn.close();
                return result;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error getting list of users with Autosend param " << e.what() << std::endl;
            result.clear();
            return result;
        }
    }
    // Включить автоотправку
    int DB::enable(int64_t id)
    {
        try
        {
            pqxx::connection conn(c_info);
            
            if (check_user_if_exists(id, conn))
            {
                pqxx::work tr{conn};
                tr.exec("UPDATE users SET isAutosend = TRUE WHERE id='" + std::to_string(id) + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error when updating user " << e.what() << std::endl;
            return 2;
        }
    }
    // Выключить автоотправку
    int DB::disable(int64_t id)
    {
        try
        {
            pqxx::connection conn(c_info);
            
            if (check_user_if_exists(id, conn))
            {
                pqxx::work tr{conn};
                tr.exec("UPDATE users SET isAutosend = FALSE WHERE id='" + std::to_string(id) + "';");
                tr.commit();
                conn.close();
                return 0;
            }
            else
            {
                conn.close();
                return 1;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] Error when updating user " << e.what() << std::endl;
            return 2;
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
// Существуел ли дата, или все же закон сильнее нас.
bool check_if_date_exists(std::string date, pqxx::work &tr)
{
    try
    {
        bool result = tr.query_value<bool>("SELECT EXISTS (SELECT 1 FROM dates WHERE date = '" + date + "') AS name_exists;");

        return result;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Unable to get existment status of date" << std::endl;

        return true;
    }
}
// Существует ли дежурный
bool check_if_exists(std::string name, pqxx::work &tr)
{
    try
    {
        bool result = tr.query_value<bool>("SELECT EXISTS (SELECT 1 FROM watchers WHERE name = '" + name + "') AS name_exists;");

        return result;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Unable to get existment status of watcher" << std::endl;

        return true;
    }
}
// Существует ли дежурный (Для удаления)
bool check_if_exists(int id, pqxx::work &tr)
{
    try
    {
        bool result = tr.query_value<bool>("SELECT EXISTS (SELECT 1 FROM watchers WHERE id = '" + std::to_string(id) + "') AS name_exists;");

        return result;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Unable to get existment status of watcher" << std::endl;

        return false;
    }
}
// Существует ли пользователь
bool check_user_if_exists(int64_t &id, pqxx::connection &c)
{
    pqxx::work tr{c};
    try
    {

        bool result = tr.query_value<bool>("SELECT EXISTS (SELECT 1 FROM users WHERE id ='" + std::to_string(id) + "') AS name_exists;");

        tr.commit();
        return result;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Unable to get existment status of user: " << e.what() << std::endl;
        tr.abort();
        return true;
    }
}
// Поиск доступного id
int get_aval_id(pqxx::work &tr)
{
    try
    {
        int id = tr.query_value<int>(SQL::get_aval_id);

        return id;
    }
    catch (std::exception &e)
    {
        std::cout << "[EE] Getting max id of watcher error:  " << e.what() << std::endl;
        return -1;
    }
}
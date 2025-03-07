#include <pqxx/pqxx>
#include <iostream>
namespace verify
{
    // Проверка наличия таблицы дежурных
    bool wch_table(pqxx::work &tr)
    {
        try
        {
            bool isExisting = tr.query_value<bool>(
                R"(SELECT EXISTS (
                    SELECT 1
                    FROM pg_tables
                    WHERE schemaname = 'public' AND tablename = 'watchers'
                );)");
            return isExisting;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            return false;
        }
    }
    // Проверка наличия таблицы пользователей
    bool user(pqxx::work &tr)
    {
        try
        {
            bool isExisting = tr.query_value<bool>(
                R"(
                SELECT EXISTS (
                    SELECT 1
                    FROM pg_tables
                    WHERE schemaname = 'public' AND tablename = 'users'
                );
                )");
            return isExisting;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            return false;
        }
    }
    bool dates(pqxx::work &tr)
    {
        try
        {
            bool isExisting = tr.query_value<bool>(
                R"(
                SELECT EXISTS (
                    SELECT 1
                    FROM pg_tables
                    WHERE schemaname = 'public' AND tablename = 'dates'
                );
                )");
            return isExisting;
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
            return false;
        }
    }
};
namespace create_tables
{
    // Создание таблицы дежурных
    void create_wch(pqxx::work &tr)
    {
        try
        {
            tr.exec(R"(CREATE TABLE IF NOT EXISTS watchers (id INTEGER, name VARCHAR(100) NOT NULL, isKilled BOOLEAN, isWas BOOLEAN, UNIQUE("id"));)");
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
        }
    }
    // Создание таблицы пользователей
    void create_users(pqxx::work &tr)
    {
        try
        {
            tr.exec(R"(CREATE TABLE IF NOT EXISTS users(id INTEGER NOT NULL,isAutosend BOOLEAN, isAdmin BOOLEAN, Username TEXT, Name TEXT, UNIQUE("id"));)");
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
        }
    }
    // Создание таблицы дат исключений
    void create_dates(pqxx::work &tr)
    {
        try
        {
            tr.exec(R"(CREATE TABLE IF NOT EXISTS dates(date TEXT, UNIQUE("date"));)");
        }
        catch (std::exception &e)
        {
            std::cout << "[EE] " << e.what() << std::endl;
        }
    }
}
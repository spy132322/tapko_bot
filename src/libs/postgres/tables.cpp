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
        }
    }
};
namespace create_tables{
    // Создание таблицы дежурных
    void create_wch(pqxx::work &tr){
        try{
            tr.exec(R"(CREATE TABLE IF NOT EXISTS watchers (name1 VARCHAR(100) NOT NULL, name2 VARCHAR(100) NOT NULL, isKilled BOOLEAN, isWas BOOLEAN);)");
        }
        catch(std::exception &e){
            std::cout << "[EE] " << e.what() << std::endl;
        }
    }
    // Создание таблицы пользователей
    void create_users(pqxx::work &tr){
        try{
            tr.exec(R"(CREATE TABLE IF NOT EXISTS users(id INTERGER NOT NULL, admin BOOLEAN, Username TEXT, Name TEXT, UNIQUE("id"));)");
        }
        catch(std::exception &e){
            std::cout << "[EE] " << e.what() << std::endl;
        }
    }
}
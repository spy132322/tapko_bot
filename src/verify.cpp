#include <pqxx/pqxx>
#include <iostream>
namespace verify
{
    // Проверка наличия таблицы дежурных
    bool user_table(pqxx::work &tr)
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
            std::cout << e.what() << std::endl;
        }
    }
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
            std::cout << e.what() << std::endl;
        }
    }
};
#include <pqxx/pqxx>
#include <iostream>
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
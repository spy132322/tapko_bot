#include <pqxx/pqxx>
#include <iostream>
bool user_table(pqxx::work &tr)
{
    try
    {
        tr.
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}
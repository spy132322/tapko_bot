#ifndef PSQLDB
#include <string>
#include <pqxx/pqxx>
namespace psql
{
    class DB
    {
    private:
        // Данные для соеденения
        std::string c_info;

    public:
        DB();
    };
};

#endif
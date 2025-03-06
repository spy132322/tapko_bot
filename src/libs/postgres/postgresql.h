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
        // Init db class settings
        DB();
        // Add new user
        void new_user(int64_t &id, std::string &name, std::string &username);
        bool check_admin(int64_t &id);
        void init_read_connection();
    };
};

#endif
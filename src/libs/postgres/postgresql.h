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
        // Verify if user is admin
        bool check_admin(int64_t &id);
        // Connection watchdog
        void connection_watchdog();
        // Stop whiles for KILL command
        void stop();
    };
};

#endif
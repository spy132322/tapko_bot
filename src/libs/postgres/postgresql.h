#ifndef PSQLDB
#include <string>
#include <pqxx/pqxx>
#include <vector>
namespace psql
{
    class DB
    {
    private:
        // Данные для соеденения
        std::string c_info;
        struct GuyData{
            int id;
            std::string Name;
            bool isKilled;
            bool isWas;
        };
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
        // Add watcher
        int add(std::string name);
        // Уч
        std::vector<GuyData> DB::list();
    };
};

#endif
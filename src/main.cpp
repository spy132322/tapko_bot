#include "libs/postgres/postgresql.h"
#include <string>
#include "libs/json.hpp"
#include "libs/messages.cpp"
#include <fstream>
#include <thread>
#include <tgbot/tgbot.h>
using json = nlohmann::json;
psql::DB db;
int main()
{
  // Импортируем ключ бота
  std::ifstream conf("config.json");
  json config = json::parse(conf);
  conf.close();

  // Инит Ботинка
  TgBot::Bot bot(config["bot"].get<std::string>());
  // Иннит комманд
  bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message)
                            { bot.getApi().sendMessage(message->chat->id, messages::hello_message); 
                                db.new_user(message->chat->id, message->chat->firstName, message->chat->username); });
  bot.getEvents().onCommand("info", [&bot](TgBot::Message::Ptr message){
    bot.getApi().sendMessage(message->chat->id, messages::info_message);
  });
  bot.getEvents().onCommand("add", [&bot](TgBot::Message::Ptr){
      
  });
}
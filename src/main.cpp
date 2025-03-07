#include <regex>
#include "libs/postgres/postgresql.h"
#include <string>
#include "libs/json.hpp"
#include "libs/messages.cpp"
#include <fstream>
#include <thread>
#include <tgbot/tgbot.h>
using json = nlohmann::json;
psql::DB db;

void run_wd();
std::thread db_wd(run_wd);
bool is_safe_input(const std::string &input);

int main()
{
  std::this_thread::sleep_for(std::chrono::seconds(2));
  // Импортируем ключ бота
  std::ifstream conf("config.json");
  json config = json::parse(conf);
  conf.close();

  // Инит Ботинка
  TgBot::Bot bot(config["bot"].get<std::string>());
  // Иннит комманд
  bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message)
  { 
    bot.getApi().sendMessage(message->chat->id, messages::hello_message); 
    db.new_user(message->chat->id, message->chat->firstName, message->chat->username);
  });
  // Вывод инфо сообщения
  bot.getEvents().onCommand("info", [&bot](TgBot::Message::Ptr message){
    bot.getApi().sendMessage(message->chat->id, messages::info_message);
  });
  //Добавить дежурного в БД
  bot.getEvents().onCommand("add", [&bot](TgBot::Message::Ptr message){
    if (db.check_admin(message->chat->id)){
      std::cout << "[II] " << message->chat->username << "has used add command " + message->text << std::endl;
    if (message-> text.size() > 5 and is_safe_input(message->text.substr(5))){
      std::string setting = message->text.substr(5);
      int result = db.add(setting);
      switch (result)
      {
      case 0:
        bot.getApi().sendMessage(message->chat->id, "Успешно добавлен дежурный " + setting);
        break;
      case 1:
        bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурный уже существует в базе данных ⚠️");
        break;
      case 2:
        bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, добавление дежурного не удачно ⚠️");
        break;
      }
    }
    else if (is_safe_input(message->text.substr(5)))
    {
      std::cout << "[WW] " << message->chat->username << "Tried to use SQL injection" << std::endl;
      bot.getApi().sendMessage(message->chat->id, "Nice try. Hah)");
    }
    else{
      bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_add);
    }
    
  }
  else{
    bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
  }
  
  });
  // Список Дежурных
  bot.getEvents().onCommand("list", [&bot](TgBot::Message::Ptr message){
    std::vector<std::string> list;
    list[0] = "📋 Список дежурных:";
    for (auto& Guy : db.list()){
      if(Guy.isKilled){
        list.push_back(std::to_string(Guy.id) + ". " + Guy.Name + " 🔴 (Не доступен)");
      }
      else if(Guy.isWas){
        list.push_back(std::to_string(Guy.id) + ". " + Guy.Name + " 🟢 (Доступен)  | Дежурил: ✅");
      } 
      else if(!Guy.isKilled and !Guy.isWas){
        list.push_back(std::to_string(Guy.id) + ". " + Guy.Name + " 🟢 (Доступен)  | Дежурил: ❌");
      }
    }
    std::string message_to = "";
    for (std::string strm : list){
      message_to = message_to + strm + "\n";
    }
    bot.getApi().sendMessage(message->chat->id, message_to);
  });
  // Удаление дежурного из БД
  bot.getEvents().onCommand("del", [&bot](TgBot::Message::Ptr message){
  if (db.check_admin(message->chat->id)){
    std::cout << "[II] " << message->chat->username << "has used add command " + message->text << std::endl;
    if (message-> text.size() > 5 and is_safe_input(message->text.substr(5))){
      std::string setting = message->text.substr(5);
      int result = db.add(setting);
      switch (result)
      {
      case 0:
        bot.getApi().sendMessage(message->chat->id, "Дежурный успешно удален  " + setting);
        break;
      case 1:
        bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурного не существует в базе данных ⚠️");
        break;
      case 2:
        bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, удаление дежурного не удачно ⚠️");
        break;
      }
    }
    else if (is_safe_input(message->text.substr(5)))
    {
      std::cout << "[WW] " << message->chat->username << "Tried to use SQL injection" << std::endl;
      bot.getApi().sendMessage(message->chat->id, "Nice try. Hah)");
    }
    else{
      bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_del);
    }
  }
  else{
    bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
  }
  
  });
  try {
    printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
    TgBot::TgLongPoll longPoll(bot);
    while (true) {
        printf("Long poll started\n");
        longPoll.start();
    }
} catch (TgBot::TgException& e) {
    printf("error: %s\n", e.what());
}
return 0;
}
bool is_safe_input(const std::string &input) {
  // Регулярное выражение для проверки на опасные символы
  std::regex dangerous_chars(R"([;\-\-'"\*\\/])");

  // Если найдены опасные символы, возвращаем false
  if (std::regex_search(input, dangerous_chars)) {
      return false;
  }

  return true;
}
void run_wd(){
  db.connection_watchdog();
}
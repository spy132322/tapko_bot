#include <regex>
#include "libs/postgres/postgresql.h"
#include <string>
#include "libs/messages.cpp"
#include <fstream>
#include <thread>
#include <tgbot/tgbot.h>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
volatile sig_atomic_t stop = 0;
bool skip = false;
bool fboot = true;
psql::DB db;
void Skip();
void noSkip();
bool is_safe_input(const std::string &input);
bool isInterger(std::string a);
bool isDate(std::string date);
void Updater();
void UpdateWatchers();
void UpdateMessage();
void sigterm(int signal);
void clearifend();
void autosender();

std::string bot_key = std::getenv("TGBOT_KEY");
// Текущее сообщение о дежурстве
std::string curr_message = "🚨 Дежурных нет. Слишком мало для создания списка дежурных.";
int guys = 0;
struct wch
{
  std::vector<int> ids;
  std::vector<std::string> names;
};
struct time_s
{
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  int milliseconds;
  int weekday;
};
wch current_watchers;
wch old_watchers;
time_s Time;
bool get_curr_time();
int main()
{
  // Инит Ботинка
  TgBot::Bot bot(bot_key);
  // Иннит комманд
  bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message)
                            { 
    bot.getApi().sendMessage(message->chat->id, messages::hello_message); 
    db.new_user(message->chat->id, message->chat->firstName, message->chat->username); });
  // Вывод инфо сообщения
  bot.getEvents().onCommand("info", [&bot](TgBot::Message::Ptr message)
                            { bot.getApi().sendMessage(message->chat->id, messages::info_message); });
  // Добавить дежурного в БД
  bot.getEvents().onCommand("add", [&bot](TgBot::Message::Ptr message)
                            {
                              if (db.check_admin(message->chat->id))
                              {
                                std::cout << "[II] " << message->chat->username << " has used add command " + message->text << std::endl;
                                if (message->text.size() > 4 and is_safe_input(message->text.substr(5)))
                                {
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
                                else if(message->text.size() <= 4)
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_add);
                                }
                                else
                                {
                                  std::cout << "[WW] " << message->chat->username << " Tried to use SQL injection" << std::endl;
                                  bot.getApi().sendMessage(message->chat->id, "Nice try. Hah)");
                                }
                              }
                              else
                              {
                                bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                              } });
  bot.getEvents().onCommand("help", [&bot](TgBot::Message::Ptr message)
                            {
                                std::cout << "[II] " + message->chat->username + " has used help command" << std::endl;
                                if (db.check_admin(message->chat->id)){
                                  bot.getApi().sendMessage(message->chat->id, messages::help_admin,nullptr,nullptr,nullptr,"markdown");
                                }
                                else
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::help_users,nullptr,nullptr,nullptr,"markdown");
                                } });
  // Включить автоотправку
  bot.getEvents().onCommand("enable", [&bot](TgBot::Message::Ptr message)
                            {
                                int result = db.enable(message->chat->id);
                                std::cout << "[II] " << message->chat->username << " has used enable command " + message->text << std::endl;
                                switch (result){
                                  case 0:
                                    bot.getApi().sendMessage(message->chat->id, "Автоотправка успешно включена.");
                                    break;
                                  case 1:
                                    bot.getApi().sendMessage(message->chat->id, "Пользователя нет в базе данных.\n Для добавление используйте комманду /start.");
                                    break;
                                  case 2:
                                    bot.getApi().sendMessage(message->chat->id, "Внутренняя ошибка сервера.\n Автоотправка не была включена.");
                                    break;
                                } });
  // Выключить автоотправку
  bot.getEvents().onCommand("disable", [&bot](TgBot::Message::Ptr message)
                            {
                                int result = db.enable(message->chat->id);
                                std::cout << "[II] " << message->chat->username << " has used disable command " + message->text << std::endl;
                                switch (result){
                                  case 0:
                                    bot.getApi().sendMessage(message->chat->id, "Автоотправка успешно выключена.");
                                    break;
                                  case 1:
                                    bot.getApi().sendMessage(message->chat->id, "Пользователя нет в базе данных.\n Для добавление используйте комманду /start.");
                                    break;
                                  case 2:
                                    bot.getApi().sendMessage(message->chat->id, "Внутренняя ошибка сервера.\n Автоотправка не была выключена.");
                                    break;
                                } });
  // Список Дежурных
  bot.getEvents().onCommand("list", [&bot](TgBot::Message::Ptr message)
                            {
                              std::cout << "[II] " << message->chat->firstName << " has used list command" << std::endl; 
                              std::vector<std::string> list;
                              list.push_back(R"(📋 Список дежурных:
```
ID |    Имя    | Состояние
                                )");
                              if (db.list()[0].Name != "Список пуст")
                              {
                                for (auto &Guy : db.list())
                                {
                                  std::string name = Guy.Name;
                                  std::string id = std::to_string(Guy.id);
                                  while(name.size() < 11){
                                    name = name + " ";
                                  }
                                  while(id.size() < 3){
                                    name = name + " ";
                                  }
                                  if (Guy.isKilled)
                                  {
                                    list.push_back(std::to_string(Guy.id) + "|" + name + "| 🔴 (Не доступен)");
                                  }
                                  if (Guy.isWas and !Guy.isKilled)
                                  {
                                    list.push_back(std::to_string(Guy.id) + "|" + name + "| 🟢 (Доступен)  | Было дежурство: ✅");
                                  }
                                  if (!Guy.isKilled and !Guy.isWas)
                                  {
                                    list.push_back(std::to_string(Guy.id) + "|" + name + "| 🟢 (Доступен)  | Было дежурство: ❌");
                                  }
                                }
                                
                              }
                              else
                                {
                                  list[0] = "📋 Список дежурных пуст. Нет данных для отображения.";
                                }
                              std::string message_to = "";
                              for (std::string strm : list)
                              {
                                message_to = message_to + strm + "\n";
                              }
                              bot.getApi().sendMessage(message->chat->id, message_to); });
  // Удаление дежурного из БД
  bot.getEvents().onCommand("del", [&bot](TgBot::Message::Ptr message)
                            {
                              if (db.check_admin(message->chat->id))
                              {
                                std::cout << "[II] " << message->chat->username << " has used add command " + message->text << std::endl;
                                if (message->text.size() > 5 and isInterger(message->text.substr(5)))
                                {

                                  std::string setting = message->text.substr(5);
                                  int result = db.del(std::stoi(setting));
                                  switch (result)
                                  {
                                  case 0:
                                    bot.getApi().sendMessage(message->chat->id, "Дежурный успешно удален с ID: " + setting);
                                    break;
                                  case 1:
                                    bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурного не существует в базе данных ⚠️");
                                    break;
                                  case 2:
                                    bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, удаление дежурного не удачно ⚠️");
                                    break;
                                  }
                                }
                                else if(message->text.size() <= 4)
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_del);
                                }
                                else {
                                  bot.getApi().sendMessage(message->chat->id, messages::wrong_params);
                                }
                              }
                              else
                              {
                                bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                              } });
  // Add date bot command
  bot.getEvents().onCommand("add_date", [&bot](TgBot::Message::Ptr message)
                            {
                              if (db.check_admin(message->chat->id))
                              {
                                std::cout << "[II] " << message->chat->username << " has used add_date command " + message->text << std::endl;
                                if (message->text.size() > 10 and isDate(message->text.substr(10)))
                                {
                                  std::string setting = message->text.substr(10);
                                  int result = db.add_date(setting);
                                  switch (result)
                                  {
                                  case 0:
                                    bot.getApi().sendMessage(message->chat->id, "Успешно добавлена дата " + setting + " в исключения");
                                    break;
                                  case 1:
                                    bot.getApi().sendMessage(message->chat->id, "⚠️ Дата уже существует в базе данных ⚠️");
                                    break;
                                  case 2:
                                    bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, добавление даты не удачно ⚠️");
                                    break;
                                  }
                                }
                                else if (!isDate(message->text.substr(10)))
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::wrong_params_add_date);
                                }
                                else
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_add_date);
                                }
                              }
                              else
                              {
                                bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                              } });
  // List dates
  bot.getEvents().onCommand("list_dates", [&bot](TgBot::Message::Ptr message)
                            {
                              std::cout << "[II] " << message->chat->firstName << " has used list_date command" << std::endl; 
                              std::vector<std::string> list;
                              list.push_back("📋 Список исключенных дат:");
                              if (db.list_dates()[0] != "LIE" and db.list_dates()[0] != "EE")
                              {
                                for(std::string a : db.list_dates()){
                                  list.push_back(a);
                                }
                                
                              }
                              else if(db.list_dates()[0] == "LIE")
                                {
                                  list[0] = "📋 Список исклеченных дат пуст. Нет данных для отображения.";
                                }
                              else{
                                list[0] = "⚠️ Внутренняя ошибка сервера, чтение списка дат не удачно ⚠️.";
                              }
                              std::string message_to = "";
                              for (std::string strm : list)
                              {
                                message_to = message_to + strm + "\n";
                              }
                              bot.getApi().sendMessage(message->chat->id, message_to); });
  // Удалиит себя дату
  bot.getEvents().onCommand("del_date", [&bot](TgBot::Message::Ptr message)
                            {
                                if (db.check_admin(message->chat->id))
                                {
                                  std::cout << "[II] " << message->chat->username << " has used del_date command " + message->text << std::endl;
                                  if (message->text.size() > 10 and isDate(message->text.substr(10)))
                                  {
                                    std::string setting = message->text.substr(10);
                                    int result = db.del_date(setting);
                                    switch (result)
                                    {
                                    case 0:
                                      bot.getApi().sendMessage(message->chat->id, "Успешно удалена дата " + setting + " из исключений");
                                      break;
                                    case 1:
                                      bot.getApi().sendMessage(message->chat->id, "⚠️ Дата не существует в базе данных ⚠️");
                                      break;
                                    case 2:
                                      bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, удаление даты не удачно ⚠️");
                                      break;
                                    }
                                  }
                                  else if (!isDate(message->text.substr(10)))
                                  {
                                    bot.getApi().sendMessage(message->chat->id, messages::wrong_params_del_date);
                                  }
                                  else
                                  {
                                    bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_del_date);
                                  }
                                }
                                else
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                                } });
  // Текущие дежурные
  bot.getEvents().onCommand("current", [&bot](TgBot::Message::Ptr message)
                            { 
                              std::cout << "[II] " << message->chat->id << " has used /current command" << std::endl;
                              bot.getApi().sendMessage(message->chat->id, curr_message); });
  bot.getEvents().onCommand("skip", [&bot](TgBot::Message::Ptr message)
                            {
                              if (db.check_admin(message->chat->id))
                              {
                                skip = true;
                                std::string names = "";
                                  for (std::string a : current_watchers.names){
                                    names = names + " " + a;
                                  }
                                  UpdateWatchers();
                                  std::cout << "[II] " << message->chat->id << " has used /skip command" << std::endl;
                                  bot.getApi().sendMessage(message->chat->id, "⚠️ Пропущены ученики:" + names + " - с отметкой, что дежурили. ⚠️" );
                              }
                              else
                              {
                                bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                              } });
  bot.getEvents().onCommand("pass", [&bot](TgBot::Message::Ptr message)
                            {
                                if (db.check_admin(message->chat->id))
                                {
                                  std::string names = "";
                                  for (std::string a : current_watchers.names){
                                    names = names + " " + a;
                                  }
                                  UpdateWatchers();
                                  std::cout << "[II] " << message->chat->id << " has used /pass command" << std::endl;
                                  bot.getApi().sendMessage(message->chat->id, "⚠️ Пропущены ученики:" + names + " - с отметкой, что дежурили. ⚠️" );
                                }
                                else
                                {
                                  bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                                } });
  // Kill
  bot.getEvents().onCommand("kill", [&bot](TgBot::Message::Ptr message)
                            {
                                  if (db.check_admin(message->chat->id))
                                  {
                                    std::cout << "[II] " << message->chat->username << " has used kill command " + message->text << std::endl;
                                    if (message->text.size() > 6 and isInterger(message->text.substr(6)))
                                    {
    
                                      std::string setting = message->text.substr(6);
                                      int result = db.Kill(std::stoi(setting));
                                      switch (result)
                                      {
                                      case 0:
                                        bot.getApi().sendMessage(message->chat->id, "Дежурный успешно заблокирован с ID: " + setting);
                                        break;
                                      case 1:
                                        bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурного не существует в базе данных ⚠️");
                                        break;
                                      case 2:
                                        bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, блокировка дежурного не удачна ⚠️");
                                        break;
                                      }
                                    }
                                    else if(message->text.size() <= 5)
                                    {
                                      bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_kill);
                                    }
                                    else {
                                      bot.getApi().sendMessage(message->chat->id, messages::wrong_params_kill);
                                    }
                                  }
                                  else
                                  {
                                    bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                                  } });
  // unkill
  bot.getEvents().onCommand("unkill", [&bot](TgBot::Message::Ptr message)
                            {
                                    if (db.check_admin(message->chat->id))
                                    {
                                      std::cout << "[II] " << message->chat->username << " has used unkill command " + message->text << std::endl;
                                      if (message->text.size() > 7 and isInterger(message->text.substr(7)))
                                      {
      
                                        std::string setting = message->text.substr(7);
                                        int result = db.unKill(std::stoi(setting));
                                        switch (result)
                                        {
                                        case 0:
                                          bot.getApi().sendMessage(message->chat->id, "Дежурный успешно разблокирован с ID: " + setting);
                                          break;
                                        case 1:
                                          bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурного не существует в базе данных ⚠️");
                                          break;
                                        case 2:
                                          bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, разблокировка дежурного не удачна ⚠️");
                                          break;
                                        }
                                      }
                                      else if(message->text.size() <= 6)
                                      {
                                        bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_unkill);
                                      }
                                      else {
                                        bot.getApi().sendMessage(message->chat->id, messages::wrong_params_unkill);
                                      }
                                    }
                                    else
                                    {
                                      bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                                    } });
  // set
  bot.getEvents().onCommand("set", [&bot](TgBot::Message::Ptr message)
                            {
                                      if (db.check_admin(message->chat->id))
                                      {
                                        std::cout << "[II] " << message->chat->username << " has used unset command " + message->text << std::endl;
                                        if (message->text.size() > 5 and isInterger(message->text.substr(5)))
                                        {
        
                                          std::string setting = message->text.substr(5);
                                          int result = db.SetWas(std::stoi(setting));
                                          switch (result)
                                          {
                                          case 0:
                                            bot.getApi().sendMessage(message->chat->id, "Установленна отметка о дежурстве на дежурного с ID: " + setting);
                                            break;
                                          case 1:
                                            bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурного не существует в базе данных ⚠️");
                                            break;
                                          case 2:
                                            bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, установка отметки о дежурстве не удачна ⚠️");
                                            break;
                                          }
                                        }
                                        else if(message->text.size() <= 4)
                                        {
                                          bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_unset);
                                        }
                                        else {
                                          bot.getApi().sendMessage(message->chat->id, messages::wrong_params_unset);
                                        }
                                      }
                                      else
                                      {
                                        bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                                      } });
  // unset
  bot.getEvents().onCommand("unset", [&bot](TgBot::Message::Ptr message)
                            {
                                        if (db.check_admin(message->chat->id))
                                        {
                                          std::cout << "[II] " << message->chat->username << " has used unset command " + message->text << std::endl;
                                          if (message->text.size() > 7 and isInterger(message->text.substr(7)))
                                          {
          
                                            std::string setting = message->text.substr(6);
                                            int result = db.UnSetWas(std::stoi(setting));
                                            switch (result)
                                            {
                                            case 0:
                                              bot.getApi().sendMessage(message->chat->id, "Убрана отметка о дежурстве на дежурного с ID: " + setting);
                                              break;
                                            case 1:
                                              bot.getApi().sendMessage(message->chat->id, "⚠️ Дежурного не существует в базе данных ⚠️");
                                              break;
                                            case 2:
                                              bot.getApi().sendMessage(message->chat->id, "⚠️ Внутренняя ошибка сервера, уборка отметки о дежурстве не удачна ⚠️");
                                              break;
                                            }
                                          }
                                          else if(message->text.size() <= 6)
                                          {
                                            bot.getApi().sendMessage(message->chat->id, messages::not_enough_params_unset);
                                          }
                                          else {
                                            bot.getApi().sendMessage(message->chat->id, messages::wrong_params_unset);
                                          }
                                        }
                                        else
                                        {
                                          bot.getApi().sendMessage(message->chat->id, messages::not_enough_rights);
                                        } });
  UpdateWatchers();
  std::signal(SIGTERM, sigterm);
  std::thread updater(Updater);
  std::thread autosend(autosender);
  while (!stop)
  {
    try
    {
      printf("[II] Bot username: %s\n", bot.getApi().getMe()->username.c_str());
      TgBot::TgLongPoll longPoll(bot);
      while (true)
      {
        longPoll.start();
      }
    }
    catch (TgBot::TgException &e)
    {
      printf("error: %s\n", e.what());
    }
  }
  return 0;
}
bool isDate(std::string date)
{
  // Проверяем формат даты (можно добавить более строгую проверку)
  if (date.size() != 10 || date[4] != '-' || date[7] != '-')
  {
    return false;
  }

  // Проверяем, что год, месяц и день — числа
  std::string year = date.substr(0, 4);
  std::string month = date.substr(5, 2);
  std::string day = date.substr(8, 2);

  if (!isInterger(year) || !isInterger(month) || !isInterger(day))
  {
    return false;
  }

  // Дополнительная проверка на корректность месяца и дня
  int monthValue = std::stoi(month);
  int dayValue = std::stoi(day);

  if (monthValue < 1 || monthValue > 12)
  {
    return false;
  }

  if (dayValue < 1 || dayValue > 31)
  {
    return false;
  }

  return true;
}
bool isInterger(std::string a)
{
  try
  {
    std::stoi(a);
    return true;
  }
  catch (std::exception &e)
  {
    return false;
  }
}
// SQL Injection test
bool is_safe_input(const std::string &input)
{
  // Регулярное выражение для проверки на опасные символы
  std::regex dangerous_chars(R"([;\-\-'"\*\\/])");

  // Если найдены опасные символы, возвращаем false
  if (std::regex_search(input, dangerous_chars))
  {
    return false;
  }

  return true;
}
// Обновление дежурных в 12 ночи
void Updater()
{

  while (!stop)
  {
    get_curr_time();

    bool can_update = true;
    if ((db.list_dates().at(0) != "LIE") and (db.list_dates().at(0) != "EE"))
    {
      for (auto &Date : db.list_dates())
      {
        if (Time.year == std::stoi(Date.substr(0, 4)) and Time.month == std::stoi(Date.substr(5, 2)) and Time.day == std::stoi(Date.substr(8, 2)))
        {
          can_update = false;
        }
      }
    }
    if (Time.weekday == 0)
    {
      can_update = false;
    }
    if (Time.hour == 0 and can_update)
    {
      std::cout << "[II] Automatic update watchers started" << std::endl;
      UpdateWatchers();
    }
    std::this_thread::sleep_for(std::chrono::minutes(30));
  }
}
// Обновление дежурных
void UpdateWatchers()
{
  clearifend();
  if (db.list().size() > guys)
  {

    int alive = 0;
    for (auto &Guy : db.list())
    {
      if (!Guy.isKilled and !Guy.isWas)
      {
        alive++;
      }
    }
    if (alive >= guys)
    {
      current_watchers.ids.clear();
      current_watchers.names.clear();
      int max = 0;
      for (auto &Guy : db.list())
      {
        bool old = false;
        if (!old_watchers.ids.empty())
        {
          for (int i : old_watchers.ids)
          {
            if (Guy.id == i)
            {
              old = true;
              break;
            }
          }
        }
        if (max != guys and !old and (!Guy.isWas and !Guy.isKilled))
        {
          max++;
          current_watchers.ids.push_back(Guy.id);
          current_watchers.names.push_back(Guy.Name);
        }
        else if (max != guys and !old and (Guy.isKilled))
        {
          max++;
        }
        if (max == guys)
        {
          break;
        }
      }
    }
    else
    {
      int max = 0;
      current_watchers.ids.clear();
      current_watchers.names.clear();
      for (auto &Guy : db.list())
      {
        bool old = false;
        if (!old_watchers.ids.empty())
        {
          for (int i : old_watchers.ids)
          {
            if (Guy.id == i)
            {
              old = true;
              break;
            }
          }
        }
        if (max != alive and (!Guy.isWas and !Guy.isKilled))
        {
          max++;
          current_watchers.ids.push_back(Guy.id);
          current_watchers.names.push_back(Guy.Name);
        }
        else if (max != alive and (Guy.isKilled))
        {
          max++;
        }
        if (max == alive)
        {
          break;
        }
      }
    }
    if (!skip)
    {
      noSkip();
    }
    else
    {
      Skip();
    }
    skip = false;
    UpdateMessage();
  }
}
void clearifend()
{
  if (!fboot)
  {
    if (current_watchers.ids.at(0) >= db.list().size() - guys)
    {
      db.clearall();
      old_watchers.ids.clear();
    }
  }
}
// обновление сообщения
void UpdateMessage()
{
  
  if (db.list()[0].Name == "Список пуст")
  {
    curr_message = "🚨 Дежурных нет. Слишком мало для создания списка дежурных.";
  }
  else
  {
    curr_message = "🚨 Текущие дежурные на сегодня:\n";
    std::cout << db.list().size() << std::endl;
    for (std::string a : current_watchers.names)
    {
      curr_message = curr_message + a + "\n";
    }
  }
}
// Обновление статуса дежуревших
void Skip()
{

  old_watchers.ids.clear();
  for (int id : current_watchers.ids)
  {
    old_watchers.ids.push_back(id);
  }
  for (std::string Name : old_watchers.names)
  {
    std::cout << Name << std::endl;
  }
}
void noSkip()
{
  if (!fboot)
  {
    for (int id : old_watchers.ids)
    {
      db.SetWas(id);
    }
  }
  old_watchers.ids.clear();
  for (int id : current_watchers.ids)
  {

    old_watchers.ids.push_back(id);
  }
  if (fboot)
  {
    fboot = false;
  }
}
// Текущее время
bool get_curr_time()
{
  try
  {
    // Получаем текущее время
    auto now = std::chrono::system_clock::now();

    // Преобразуем время в time_t
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // Преобразуем time_t в локальное время
    std::tm now_tm = *std::localtime(&now_time_t);

    // Получаем миллисекунды
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) %
                        1000;

    // Заполняем структуру Time
    Time.year = now_tm.tm_year + 1900;        // Год (начиная с 1900)
    Time.month = now_tm.tm_mon + 1;           // Месяц (0-11 -> 1-12)
    Time.day = now_tm.tm_mday;                // День месяца
    Time.hour = now_tm.tm_hour;               // Часы
    Time.minute = now_tm.tm_min;              // Минуты
    Time.second = now_tm.tm_sec;              // Секунды
    Time.milliseconds = milliseconds.count(); // Миллисекунды
    Time.weekday = now_tm.tm_wday;            // День недели
    return true;
  }
  catch (std::exception &e)
  {
    std::cout << "[EE] Error reading time " << e.what() << std::endl;
    return false;
  }
}
void sigterm(int signal)
{
  if (signal == SIGTERM)
  {
    std::cout << "[II] Получен сигнал терминации, выключаю проццессы." << std::endl;
    stop = 1;
  }
}
// Авто отправлялка
void autosender()
{
  int aminute = 0;
  int ahour = 0;
  if(std::getenv("AUTO_HOUR") and std::getenv("AUTO_MINUTE")){
  ahour = std::stoi(std::getenv("AUTO_HOUR"));
  aminute = std::stoi(std::getenv("AUTO_MINUTE"));
  }else{
    exit(-1);
  }
  TgBot::Bot bot(bot_key);
  std::cout << "[II] Starting autosend thread" << std::endl;
  while (!stop)
  {
    get_curr_time();

    if (Time.second == 0 or Time.second > 40)
    {
      std::cout << "[II] Balancing time on autosender +20 sec" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(20));
    }
    // Выбирал время Святик!!!!!
    if (!db.list_users().empty() and Time.hour == ahour and Time.minute == aminute)
    {
      std::cout << "[II] Running autosend thread" << std::endl;
      for (int64_t id : db.list_users())
      {
        std::cout << "[II] Sending current to " << id << std::endl;
        bot.getApi().sendMessage(id, curr_message);
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(60));
  }
}
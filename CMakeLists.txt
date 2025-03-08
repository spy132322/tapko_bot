# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(tapko_bot)

# Указываем стандарт C++ (C++17 в вашем случае)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем пути к заголовочным файлам
include_directories(/usr/local/include)

# Находим необходимые библиотеки
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Указываем исходные файлы
set(SOURCES
    src/main.cpp
    src/libs/postgres/postgresql.cpp
)

# Создаем исполняемый файл
add_executable(main ${SOURCES})

# Подключаем библиотеки
target_link_libraries(main
    pqxx
    pq
    TgBot
    ${Boost_SYSTEM_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)
install(TARGETS main DESTINATION /usr/bin)
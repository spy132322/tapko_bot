cmake_minimum_required(VERSION 3.10)

project(tapko_bot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/usr/local/include)


find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)


set(SOURCES
    src/main.cpp
    src/libs/postgres/postgresql.cpp
)

add_executable(tapok_bot ${SOURCES})


target_link_libraries(tapok_bot
    pqxx
    pq
    TgBot
    ${Boost_SYSTEM_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)
install(TARGETS tapok_bot DESTINATION /usr/bin)
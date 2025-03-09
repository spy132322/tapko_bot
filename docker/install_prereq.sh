#!/bin/sh

# !!! ЗАПУСКАТЬ НЕ НА ХОСТЕ !!!
apt update
apt upgrade -y
apt update && apt install -y locales && \
    locale-gen en_US.UTF-8 && \
    locale-gen ru_RU.UTF-8 && \
    update-locale LANG=en_US.UTF-8
apt install -y build-essential g++ make binutils cmake libboost-system-dev libssl-dev zlib1g-dev libcurl4-openssl-dev cmake git libpq-dev postgresql-server-dev-all libboost-dev libssl-dev pkg-config libgtest-dev libgmock-dev doxygen
cd /
git clone https://github.com/reo7sp/tgbot-cpp
cd /tgbot-cpp
cmake .
make -j4
make install
cd /
git clone https://github.com/jtv/libpqxx.git /libpqxx
cd /libpqxx
cmake .
cmake --build .
cmake --install .
cd /tapko_bot
cmake .
make
make install
cd /



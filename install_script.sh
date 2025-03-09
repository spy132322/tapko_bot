#!/bin/sh
apt update
apt upgrade -y
apt update && apt install -y locales && \
    locale-gen en_US.UTF-8 && \
    locale-gen ru_RU.UTF-8 && \
    update-locale LANG=en_US.UTF-8
apt install -y build-essential g++ make binutils cmake libboost-system-dev libssl-dev zlib1g-dev libcurl4-openssl-dev cmake git libpq-dev postgresql-server-dev-all libboost-dev libssl-dev pkg-config libgtest-dev libgmock-dev doxygen
cd tgbot-cpp
cmake .
make -j4
sudo make install
cd ..
cd libpqxx
cmake .
cmake --build .
sudo cmake --install .
cd ..
cmake .
make
sudo make install
sudo cp tapko_bot.service /lib/systemd/system
sudo cp env.conf /etc/tapko_bot/tapko_bot.conf


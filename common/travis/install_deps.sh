#!/bin/sh
sudo apt-add-repository -y "ppa:george-edison55/george-edison"
sudo apt-get -qq update
sudo apt-get install -y cmake cmake-data libboost-dev
sudo apt-get install -y libxcb1-dev libxcb-util0-dev libxcb-image0-dev libxcb-randr0-dev libxcb-ewmh-dev libxcb-icccm4-dev xcb-proto python-xcbgen libfreetype6-dev
sudo apt-get install -y i3-wm libiw-dev libasound2-dev libmpdclient-dev

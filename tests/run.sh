#!/bin/sh
# Host tests of the model (no LVGL, no Tessera): sh tests/run.sh
set -e
cd "$(dirname "$0")/.."
c++ -std=c++17 -Wall -Werror -I components/nas_power tests/test_model.cpp components/nas_power/model.cpp -o tests/run
tests/run

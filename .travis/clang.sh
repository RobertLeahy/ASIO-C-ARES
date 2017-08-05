#!/bin/bash
set -x
rm /usr/local/clang-3.5.0/bin/clang++
ln -s /usr/bin/clang++-4.0 /usr/bin/clang++

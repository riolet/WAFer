#!/bin/sh
make clean
make&&PORT=$1 ./server

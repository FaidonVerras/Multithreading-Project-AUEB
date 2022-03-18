#!/usr/bin/bash

gcc pizza_prep.c -pthread -o pizza_prep
./pizza_prep 100 1000

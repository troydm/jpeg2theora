#!/usr/bin/env bash

gcc -O3 -I/usr/local/include -L/usr/local/lib -lm -logg -ltheora -ltheoraenc -ltheoradec -lpcre -ljpeg -o jpeg2theora main.c load.c encode.c



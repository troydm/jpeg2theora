jpeg2theora
===========

jpeg2theora is simple jpeg sequence to theora video encoder that is written in C and licensed under GPLv3

Author: "Dmitry Geurkov" <dmitry_627@mail.ru>

Depends on
----------

    libtheora 1.1.1
    libpcre
    libjpeg

License
-------
    
    GPLv3

How to INSTALL
--------------

    ./build.sh && sudo cp ./jpeg2theora /usr/local/bin/

Usage
-----

    jpeg2theora 0.1 - Copyright (C) 2011 Open IP Cam Project
    Usage: jpeg2theora [options] directory pattern
      -o --output <filename.ogv>     file name for encoded output;
                                     If this option is not given, the
                                     compressed data is sent to stdout.

      -d --denoise  <n>              Denoise filter (0-100)
      -w --width  <n>                Image width
      -h --height <n>                Image height
      -v --video-quality <n>         Theora quality selector from 0 to 10
      -k --keyframe-freq <n>         Keyframe frequency
  
Example
-------

    jpeg2theora -w 640 -h 480 -v 10 -d 50 -k 5 -o output.ogv . ".*\.jpg"

/* This file is part of Open IP Cam Project.
   Copyright (C) 2011 Open IP Cam Project http://openipcam.sf.net
   
   jpeg2theora - jpeg sequence to theora ogv video encoder
   Author: "Dmitry Geurkov" <dmitry_627@mail.ru>
   
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>

#define JPEG2THEORA_VERSION "0.1"
#define JPEG2THEORA_COPYRIGHT "Copyright (C) 2011 Open IP Cam Project"

typedef unsigned char byte;

double rint(double x);


// JPEG sequence loading
int load(char* dir, char* pattern);
byte* get_frame(int width, int height, int denoise);
int is_last_frame();
void unload();

void filter_plane(int width, int height, int denoise, byte* buffer);
void copy_plane(int width, int height,byte* src, byte* dst);
void encode(int width, int height, int video_quality, int denoise, int keyframe, FILE* outputFile);


#endif

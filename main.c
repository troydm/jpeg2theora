/* This file is part of Open IP Cam Project.
   Copyright (C) 2011 Open IP Cam Project http://openipcam.sf.net
   
   jpeg2theora - jpeg sequence to theora ogv video encoder
   Author: "Dmitry Geurkov" <d.geurkov@gmail.com>
   
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

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

double rint(double x){
  if (x < 0.0)
    return (double)(int)(x - 0.5);
  else
    return (double)(int)(x + 0.5);
}

void usage(){
	printf("jpeg2theora %s - %s\nUsage: jpeg2theora [options] directory pattern\n"
	"  -o --output <filename.ogv>     file name for encoded output;\n"
    "                                 If this option is not given, the\n"
    "                                 compressed data is sent to stdout.\n\n"
	"  -d --denoise  <n>              Denoise filter (0-100)\n"
	"  -w --width  <n>                Image width\n"
	"  -h --height <n>                Image height\n"
	"  -v --video-quality <n>         Theora quality selector from 0 to 10\n"
	"  -k --keyframe-freq <n>         Keyframe frequency\n", JPEG2THEORA_VERSION, JPEG2THEORA_COPYRIGHT
	);
}

const char *optstring = "k:v:o:d:w:h:\1\2";
struct option options [] = {
	{"output",required_argument,NULL,'o'},
	{"denoise",required_argument,NULL,'d'},
	{"width",required_argument,NULL,'w'},
	{"height",required_argument,NULL,'h'},
	{"keyframe-freq",required_argument,NULL,'k'},
	{"video-quality",required_argument,NULL,'v'},
	{NULL,0,NULL,0}
};

int main(int argc, char** argv){

  if(argc < 3){
	usage();
	exit(1);
  }
  
  FILE *outfile = stdout;  
  
  int ca,long_option_index;
  int w,h,keyframes,video_q,denoise;
  
  while((ca=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
  	switch(ca){
  		case 'o':
	      outfile=fopen(optarg,"wb");
	      if(outfile==NULL){
	        fprintf(stderr,"Unable to open output file '%s'\n", optarg);
	        exit(1);
	      }
	      break;
  		case 'k':
  			keyframes = rint(atof(optarg));
  			if(keyframes<1 || keyframes>2147483647){
		        fprintf(stderr,"Illegal keyframe frequency\n");
		        exit(1);
		     }
  			break;  
  		case 'd':
  			denoise = atof(optarg);
  			break;
  		case 'w':
  			w = atof(optarg);
  			break;
  		case 'h':
  			h = atof(optarg);
  			break;
  		case 'v':
		    video_q=(int)rint(6.3*atof(optarg));
		    if(video_q<0 || video_q>63){
		      fprintf(stderr,"Illegal video quality (choose 0 through 10)\n");
		      exit(1);
		    }
		    break;	
  	}
  }
  
  // Loading jpeg stream
  load(argv[argc-2],argv[argc-1]);
  
  //get_frame(640,480);
  encode(w, h, video_q, denoise, keyframes, outfile);
  
  // Unloading files
  unload();
  
  // Closing output file
  if(outfile && outfile!=stdout)fclose(outfile);
  
  return 0;
}

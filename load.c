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

#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <pcre.h>
#include <jpeglib.h>
#include <jerror.h>

static char** files;
static unsigned int fileCount;
static unsigned int lastFile = 0;

int load(char* dir, char* pattern){
	pcre *re;
	struct stat     statbuf;
    struct dirent   *dirp;
    DIR             *dp;
    int             ret;
    char            *ptr;
	int erroroffset, rc, size;
	int ovector[30];
	char buf[256] = "";
    const char* error;

    if (lstat(dir, &statbuf) < 0){ /* stat error */
        printf("Couldn't find directory: %s\n",dir);
		exit(1);
	}
    if (S_ISDIR(statbuf.st_mode) == 0){ /* not a directory */
        printf("%s is not a directory\n",dir);
		exit(1);
	}
	
	if ((dp = opendir(dir)) == NULL){
	    printf("couldn't opendir %s\n",dir);
		exit(1);
	}
	
	printf("Loading directory: %s with pattern: %s\n",dir,pattern);
	
	// Compiling pattern
	re = pcre_compile(pattern, 0, &error, &erroroffset, NULL);	
	if (re == NULL){
		printf("Invalid pattern specified: %s\n",pattern);
		exit(1);
	}
	
	// int file count
	fileCount = 0;
	while ((dirp = readdir(dp)) != NULL) {
         if (strcmp(dirp->d_name, ".") == 0 ||
             strcmp(dirp->d_name, "..") == 0)
                 continue;
		
		buf[0] = '\0';
		strcat(buf,dir);
		strcat(buf,"/");
		strcat(buf,dirp->d_name);
		
		if (lstat(buf, &statbuf) < 0 || S_ISREG(statbuf.st_mode) == 0 || statbuf.st_size == 0 || (statbuf.st_mode & S_IRWXU) == 0){
			continue;
		}
		
		rc = pcre_exec(
           re,             /* result of pcre_compile() */
           NULL,           /* we didn't study the pattern */
           dirp->d_name,  /* the subject string */
           strlen(dirp->d_name),             /* the length of the subject string */
           0,              /* start at offset 0 in the subject */
           0,              /* default options */
           ovector,        /* vector of integers for substring information */
           30);            /* number of elements (NOT size in bytes) */

		if(rc == PCRE_ERROR_NOMATCH){
			// for debugging printf("pcre_exec of pattern:[%s] %s %d\n",pattern,dirp->d_name,rc);
			continue;
		}
		
		
		fileCount++;
	}
	
	rewinddir(dp);
	
	files = malloc(sizeof(char*)*fileCount);
	
	int i = 0;
	while ((dirp = readdir(dp)) != NULL) {
         if (strcmp(dirp->d_name, ".") == 0 ||
             strcmp(dirp->d_name, "..") == 0)
                 continue;
		
		buf[0] = '\0';
		strcat(buf,dir);
		strcat(buf,"/");
		strcat(buf,dirp->d_name);
		
		if (lstat(buf, &statbuf) < 0 || S_ISREG(statbuf.st_mode) == 0 || statbuf.st_size == 0 || (statbuf.st_mode & S_IRWXU) == 0){
			continue;
		}
		
		rc = pcre_exec(
           re,             /* result of pcre_compile() */
           NULL,           /* we didn't study the pattern */
           dirp->d_name,  /* the subject string */
           strlen(dirp->d_name),             /* the length of the subject string */
           0,              /* start at offset 0 in the subject */
           0,              /* default options */
           ovector,        /* vector of integers for substring information */
           30);            /* number of elements (NOT size in bytes) */

		if(rc == PCRE_ERROR_NOMATCH){
			// for debugging printf("pcre_exec of pattern:[%s] %s %d\n",pattern,dirp->d_name,rc);
			continue;
		}
		
		files[i] = malloc(256*sizeof(char));
		
		strcpy(files[i], buf);
		
		i++;
	}
	
	pcre_free(re);
	
	printf("files names loaded: %d\n",fileCount);

	// Sorting files
	if(fileCount > 1){
		i = 0;
		while(i < fileCount-1){
			int j = i+1;
			while(j < fileCount){
				int c = strcmp(files[i],files[j]);
				if(c > 0){
					char* b = files[i];
					files[i] = files[j];
					files[j] = b;
				}
				j++;
			}
			i++;
		}
	}
	
	/*
	i = 0;
	while(i < fileCount){
		printf("file: %s\n",files[i]);
		i++;
	}*/
	
	if (closedir(dp) < 0){
	    printf("couldn't closedir %s\n",dir);
		exit(1);
	}

	return fileCount;
}

static byte* buffer = NULL;
static byte* buffer_line = NULL;
static byte* buffer_plane = NULL;

int is_last_frame(){
	return lastFile == fileCount ? 1 : 0;
}

byte* get_frame(int width, int height,int denoise){
	if(lastFile >= fileCount)
		return NULL;
	if(buffer == NULL){
		buffer = malloc(width*height*3);
		buffer_line = malloc(width*3);
		if(denoise > 0) buffer_plane = malloc(width*height);
	}
		
	char* filename = files[lastFile];lastFile++;
	FILE* fd;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_decompress (&cinfo);
	
	if ((fd = fopen(filename, "rb")) == 0)
		return NULL;
	
	jpeg_stdio_src (&cinfo, fd);
	jpeg_read_header (&cinfo, TRUE);
	
	if ((cinfo.image_width != width) || (cinfo.image_height != height))
		return NULL;
		
	if (cinfo.out_color_space == JCS_GRAYSCALE)
		return NULL;
	
	cinfo.do_block_smoothing = TRUE;
	cinfo.do_fancy_upsampling = TRUE;
	cinfo.out_color_space = JCS_YCbCr;
	
	jpeg_start_decompress(&cinfo);

	int wxh = width*height; 
	int pi = 0;
	
	while (cinfo.output_scanline < cinfo.output_height){
		jpeg_read_scanlines(&cinfo, &buffer_line, 1);
		
		int i = 0;
		while(i < width*3){
			
			buffer[pi] = buffer_line[i];
			buffer[pi+wxh] = buffer_line[i+1];
			buffer[pi+wxh*2] = buffer_line[i+2];			
			
			i = i+3;
			pi++;
		}
		
	}
	
	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);
	
	if(denoise > 0){
		filter_plane(width,height,denoise,buffer);
		//filter_plane(width,height,denoise,buffer+width*height);
		//filter_plane(width,height,denoise,buffer+width*height*2);
	}
	
	fclose(fd);
	
	return buffer;
}

void filter_plane(int width, int height, int denoise, byte* bf){
	int j = 1;
	int k = 1;
	while(j < height-2){
		k = 1;
		while(k < width-2){
			int b = 2;
			int c = b*6+4;
			buffer_plane[k+j*width] = (int)((1.0f/c)*(
			
			bf[k-1+(j-1)*width]+
			b*bf[k+(j-1)*width]+
			bf[k+1+(j-1)*width]+
			
			b*bf[k-1+j*width]+
			b*b*bf[k+j*width]+
			b*bf[k+1+j*width]+
			
			bf[k-1+(j+1)*width]+
			b*bf[k+(j+1)*width]+
			bf[k+1+(j+1)*width]
			
			));
			
			//printf("%d,",buffer_plane[j+k*width]);
			
			k++;
		}
		//printf("\n");
		j++;
	}
	
	j = 1;
	while(j < height-2){
		k = 1;
		while(k < width-2){
			
			//int dif = bf[k+j*width] - buffer_plane[k+j*width];
			
			bf[k+j*width] = (denoise*buffer_plane[k+j*width]+(100-denoise)*bf[k+j*width])/100;
			
			//printf(" %d ",);
			
			k++;
		}
		//printf("\n");
		j++;
	}	
}

void copy_plane(int width, int height,byte* src, byte* dst){
	int j = 0;
	int k = 0;
	while(j < height){
		k = 0;
		while(k < width){
			dst[k+j*width] = src[k+j*width];
			k++;
		}
		j++;
	}
}

void unload(){
	int i = 0;
	while(i < fileCount){
		free(files[i]);
		i++;
	}
	
	free(files);
	if(buffer != NULL){
		free(buffer);
		free(buffer_line);
		if(buffer_plane != NULL) free(buffer_plane);
	}
}

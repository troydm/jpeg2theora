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
#include <stdio.h>
#include <stdlib.h>
#include <theora/theora.h>
#include <theora/theoraenc.h>

static int frame_w=0;
static int frame_h=0;
static int pic_w=0;
static int pic_h=0;
static int pic_x=0;
static int pic_y=0;
static int video_q;
static int keyframe_frequency;
static FILE* outfile = NULL;

static int dst_c_dec_h=1;
static int dst_c_dec_v=1;
static int speed=-1;
static int buf_delay=-1;

static int video_r=-1;
static int video_q=-1;

static ogg_stream_state to;
static ogg_stream_state vo;
static ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
static ogg_packet       op; 

static th_enc_ctx      *td;
static th_info          ti;
static th_comment       tc;
static int ret;

static int ilog(unsigned _v){
  int ret;
  for(ret=0;_v;ret++)_v>>=1;
  return ret;
}

void encode(int width, int height, int video_quality, int denoise, int keyframe, FILE* outputFile){
	
	pic_w = width;
	pic_h = height;
	video_q = video_quality;
	keyframe_frequency = keyframe;
	outfile = outputFile;
	
	if(video_q==-1){
      if(video_r>0)
        video_q=0;
      else
        video_q=48;
    }
	
	/* Set up Ogg output stream */
	srand(time(NULL));
	ogg_stream_init(&to,rand()); /* oops, add one ot the above */
	
	
	/* Theora has a divisible-by-sixteen restriction for the encoded frame size */
    /* scale the picture size up to the nearest /16 and calculate offsets */
    frame_w=pic_w+15&~0xF;
    frame_h=pic_h+15&~0xF;
    /*Force the offsets to be even so that chroma samples line up like we
       expect.*/
    pic_x=frame_w-pic_w>>1&~1;
    pic_y=frame_h-pic_h>>1&~1;
    th_info_init(&ti);
    ti.frame_width=frame_w;
    ti.frame_height=frame_h;
    ti.pic_width=pic_w;
    ti.pic_height=pic_h;
    ti.pic_x=pic_x;
    ti.pic_y=pic_y;
    ti.fps_numerator=keyframe_frequency;
    ti.fps_denominator=1;
    ti.aspect_numerator= -1;
    ti.aspect_denominator= -1;
    ti.colorspace=TH_CS_UNSPECIFIED;
    /*Account for the Ogg page overhead.
      This is 1 byte per 255 for lacing values, plus 26 bytes per 4096 bytes for
       the page header, plus approximately 1/2 byte per packet (not accounted for
       here).*/
    ti.target_bitrate=(int)(64870*(ogg_int64_t)video_r>>16);
    ti.quality=video_q;
    ti.keyframe_granule_shift=ilog(keyframe_frequency-1);
    
    /*if(dst_c_dec_h==2){
      if(dst_c_dec_v==2)ti.pixel_fmt=TH_PF_420;
      else ti.pixel_fmt=TH_PF_422;
    }
    else ti.pixel_fmt=TH_PF_444;*/
    ti.pixel_fmt=TH_PF_444;
    
    td=th_encode_alloc(&ti);
    th_info_clear(&ti);
    /* setting just the granule shift only allows power-of-two keyframe
       spacing.  Set the actual requested spacing. */
    ret=th_encode_ctl(td,TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,&keyframe_frequency,sizeof(keyframe_frequency-1));
    if(ret<0){
      fprintf(stderr,"Could not set keyframe interval to %d.\n",(int)keyframe_frequency);
    }
    
    
    /*Now we can set the buffer delay if the user requested a non-default one
       (this has to be done after two-pass is enabled).*/
    if(buf_delay>=0){      
      ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,&buf_delay,sizeof(buf_delay));
      if(ret<0){
        fprintf(stderr,"Warning: could not set desired buffer delay to %d %d.\n",buf_delay,ret);
      }
    }
    
    
    /*Speed should also be set after the current encoder mode is established,
       since the available speed levels may change depending.*/
    if(speed>=0){
      int speed_max;
      int ret;
      ret=th_encode_ctl(td,TH_ENCCTL_GET_SPLEVEL_MAX,&speed_max,sizeof(speed_max));
      if(ret<0){
        fprintf(stderr,"Warning: could not determine maximum speed level.\n");
        speed_max=0;
      }
      ret=th_encode_ctl(td,TH_ENCCTL_SET_SPLEVEL,&speed,sizeof(speed));
      if(ret<0){
        fprintf(stderr,"Warning: could not set speed level to %i of %i\n",
         speed,speed_max);
        if(speed>speed_max){
          fprintf(stderr,"Setting it to %i instead\n",speed_max);
        }
        ret=th_encode_ctl(td,TH_ENCCTL_SET_SPLEVEL,&speed_max,sizeof(speed_max));
        if(ret<0){
          fprintf(stderr,"Warning: could not set speed level to %i of %i\n",
           speed_max,speed_max);
        }
      }
    }
    
    /* write the bitstream header packets with proper page interleave */
    th_comment_init(&tc);
    /* first packet will get its own page automatically */
    if(th_encode_flushheader(td,&tc,&op)<=0){
      fprintf(stderr,"Internal Theora library error.\n");
      exit(1);
    }
    
    ogg_stream_packetin(&to,&op);
    if(ogg_stream_pageout(&to,&og)!=1){
      fprintf(stderr,"Internal Ogg library error.\n");
      exit(1);
    }
    fwrite(og.header,1,og.header_len,outfile);
    fwrite(og.body,1,og.body_len,outfile);
    
    /* create the remaining theora headers */
    for(;;){
      ret=th_encode_flushheader(td,&tc,&op);
      if(ret<0){
        fprintf(stderr,"Internal Theora library error.\n");
        exit(1);
      }
      else if(!ret)break;
      ogg_stream_packetin(&to,&op);
    }
    
    for(;;){
       int result = ogg_stream_flush(&to,&og);
       if(result<0){
         /* can't get here */
         fprintf(stderr,"Internal Ogg library error.\n");
         exit(1);
       }
       if(result==0)break;
       fwrite(og.header,1,og.header_len,outfile);
       fwrite(og.body,1,og.body_len,outfile);
    }
    
    double timebase=0;
    int vkbps=0;
    ogg_int64_t audio_bytesout=0;
  	ogg_int64_t video_bytesout=0;
  	int pic_sz = pic_w*pic_h;
  	byte* buffer = NULL;
    
    for(;;){
      ogg_page videopage;
      ogg_packet op;
      
      // encode packet
      
      th_ycbcr_buffer ycbcr;
      
      while(1){
      	if(ogg_stream_pageout(&to,&videopage)>0)
	    	break;
	    
        if(ogg_stream_eos(&to))
	    	break;
	    	
	    if(th_encode_packetout(td,0,&op)>0)
	    	ogg_stream_pageout(&to,&videopage);
	    
	    buffer = get_frame(width,height,denoise);
	    
      	if(buffer == NULL){
	    	break;
	    }
	    
	    ycbcr[0].width=pic_w;
		ycbcr[0].height=pic_h;
		ycbcr[0].stride=pic_w;
		ycbcr[0].data=buffer;
	    
	    ycbcr[1].width=pic_w;
		ycbcr[1].height=pic_h;
		ycbcr[1].stride=pic_w;
		ycbcr[1].data=buffer+pic_sz;
	    
	    ycbcr[2].width=pic_w;
		ycbcr[2].height=pic_h;
		ycbcr[2].stride=pic_w;
		ycbcr[2].data=buffer+pic_sz*2;
	    
	    ret = th_encode_ycbcr_in(td,ycbcr);
	    int last = is_last_frame();
    	if(ret == 0) th_encode_packetout(td,last,&op);
	    	    
	    if(ret<=0){
	    	if(last == 1){
	    		ogg_stream_packetin(&to,&op);
	    		if(ogg_stream_pageout(&to,&videopage)>0)
	    			break;
	    	}
	    }
	    ogg_stream_packetin(&to,&op);
      }

      // end encoding
      
      double timebase = th_granule_time(td,ogg_page_granulepos(&videopage));
      video_bytesout+=fwrite(videopage.header,1,videopage.header_len,outfile);
      video_bytesout+=fwrite(videopage.body,1,videopage.body_len,outfile);
      
      if(timebase > 0){
   	  	int hundredths=(int)(timebase*100-(long)timebase*100);
        int seconds=(long)timebase%60;
        int minutes=((long)timebase/60)%60;
        int hours=(long)timebase/3600;
      	vkbps = (int)rint(video_bytesout*8./timebase*.001);
   	
   		fprintf(stderr,
                "\r      %d:%02d:%02d.%02d video: %dkb/ps %.2fMB size                 ",
                hours,minutes,seconds,hundredths,vkbps,video_bytesout/(1048576.0));
      }
      
      if(is_last_frame() == 1)
      	break;
    }
    
    th_encode_free(td);
    
    ogg_stream_clear(&to);
    th_comment_clear(&tc);
    fprintf(stderr,"\r   \ndone.\n\n");
}


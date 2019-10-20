/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   * Copyright (C) 2019 All rights reserved.
   * File Name: camera.h
   * Author: yuanxiao
   * Mail: 1278034477@qq.com 
   * Created Time: 2019.08.22
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifndef _CAMERA_H
#define _CAMERA_H



#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/types.h>



#include <linux/videodev2.h>
#include "convert.h"
 

#define FILE_VIDEO	"/dev/video0"


#define  IMAGEWIDTH    640
#define  IMAGEHEIGHT   480

static  int fd;
static   struct   v4l2_capability   cap;


struct v4l2_buffer buf;

struct v4l2_fmtdesc fmtdesc;
struct v4l2_format fmt;
struct v4l2_streamparm fps;  
struct v4l2_requestbuffers req;
enum v4l2_buf_type type;

unsigned char rgb_buffer[IMAGEWIDTH*IMAGEHEIGHT*3];

struct buffer
{
	void * start;
	unsigned int length;
} *buffers;

int Open_Cam(char *dev);
int Init_Cam(int fd);
int DQ_JPEG2Buf(int fd, void *picbuf);
int Q_Pic2Buf(int fd);
int Close_Cam(int fd);


#endif



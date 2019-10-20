/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   * Copyright (C) 2019 All rights reserved.
   * File Name: server.h
   * Author: yuanxiao
   * Mail: 1278034477@qq.com 
   * Created Time: 2019.08.19
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef __SERVER_H
#define __SERVER_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

unsigned char Get_temp(unsigned char * buf);	   //获取温度值
unsigned char Get_hum(unsigned char * buf);		   //获取湿度
unsigned short Get_illuminate(unsigned char * buf);//获取光照


void Ctl_dev(int portfd, char *cmd);//发送控制命令cmd到串口portfd

void * Ctl_request(void *Ctl_fd);//控制线程处理函数

void * Send_Msg(void *arg);//数据发送线程处理函数

void Data_com(unsigned char* Data_buf, unsigned char* temperature, unsigned char* humidity, unsigned char* illuminate);

#endif


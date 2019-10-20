/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   * Copyright (C) 2019 All rights reserved.
   * File Name: Searial_port.c
   * Author: yuanxiao
   * Mail: 1278034477@qq.com 
   * Created Time: 2019.08.19
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "server.h"


int Searialport_Open(char *Searialport)
{
	int fd=-1;
	struct termios new_cfg,old_cfg;
	
	fd = open(Searialport, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(fd == -1)
	{
		perror("open");
		return -1;
	}
 
	//preserve the old configuration
	if(tcgetattr(fd,&old_cfg)!=0)
		return fd;
	//set mode
    new_cfg.c_cflag |=CLOCAL |CREAD;
 
	//set baudrate
	cfsetispeed(&new_cfg,B115200);
	cfsetospeed(&new_cfg,B115200);
   
	//set data bit
	new_cfg.c_cflag&=~CSIZE;
	new_cfg.c_cflag |=CS8;
                                                                            
    //set stop bit
	new_cfg.c_cflag &=~(CSTOPB);

	new_cfg.c_cflag &= ~PARENB;
	
	//set others
    new_cfg.c_cc[VTIME]=0;
    new_cfg.c_cc[VMIN]=0;
    //start
    if(-1==tcsetattr(fd,TCSANOW,&new_cfg))
            return fd;
	return fd;
}








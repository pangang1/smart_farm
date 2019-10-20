/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   * Copyright (C) 2019 All rights reserved.
   * File Name: server.c
   * Author: yuanxiao
   * Mail: 1278034477@qq.com 
   * Created Time: 2019.08.19
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "server.h"
#include "camera.h"

unsigned char led_on[]   = {0xdd, 0x03, 0x24, 0x00, 0x00};//打开led   ledon
unsigned char led_off[]  = {0xdd, 0x03, 0x24, 0x00, 0x01};//关闭led    ledoff
unsigned char fan_on[]   = {0xdd, 0x03, 0x24, 0x00, 0x04};//打开风扇  fanon
unsigned char fan_off[]  = {0xdd, 0x03, 0x24, 0x00, 0x08};//关闭风扇  fanoff
unsigned char beep_on[]  = {0xdd, 0x03, 0x24, 0x00, 0x02};//打开蜂鸣器 beepon
unsigned char beep_off[] = {0xdd, 0x03, 0x24, 0x00, 0x03};//关闭蜂鸣器 beepoff

unsigned char jpeg_buffer[640*480*4]; //存储jpeg图片

pthread_t tid1, tid2;
pthread_mutex_t mutex;


struct fdset{
	int ctlsoc;
	int portfd;
};

void foo(int arg){}//空函数，防止管道破裂关闭服务器

int main(void)
{	
	//初始化摄像头
	int camfd  = Open_Cam("/dev/video0");
	Init_Cam(camfd);
	printf("camera Ok!\n");

	//初始化串口
	int fd = Searialport_Open("/dev/ttyUSB0");

	printf("open USB ok\n");

	pid_t ctl_id = fork();//创建摄像头进程
	if(ctl_id < 0)
	{
		perror("fork");
	}
	else if(ctl_id == 0)  //子进程处理摄像头信息
	{
		int cam_soc = socket(AF_INET, SOCK_STREAM, 0);//默认TCP
		if(cam_soc < 0)
		{
			fprintf(stderr, "cam_pro socket error!\n");
			exit(-1);
		}

		int on = 1;
		setsockopt(cam_soc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		//绑定
		struct sockaddr_in cam_addr;
		bzero(&cam_addr, sizeof(cam_addr));
		cam_addr.sin_family = AF_INET;
		cam_addr.sin_port = htons(7777); //控制端口
		cam_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("192.168.2.");
		if(bind(cam_soc, (struct sockaddr *)&cam_addr, sizeof(cam_addr)) < 0)
		{
			fprintf(stderr, "cam_pro bind error!\n");
			exit(-1);
		}

		//监听
		listen(cam_soc, 10);
		signal(SIGPIPE, foo);

		int size = 0;
		char size_buf[10] = {0};
		

		while(1)
		{
			printf("wait camera!\n");
			int cam_cli = accept(cam_soc, NULL, NULL);
	   		
			//前面四张图片丢弃
			int i = 0;
			for(i = 0; i < 4; i++)
			{
				DQ_JPEG2Buf(camfd, jpeg_buffer);
				Q_Pic2Buf(camfd);
			}

			while(1)
			{
				bzero(jpeg_buffer, sizeof(jpeg_buffer));
				bzero(size_buf, sizeof(size_buf));
				size = DQ_JPEG2Buf(camfd, jpeg_buffer);  //出队
				Q_Pic2Buf(camfd);
				
				printf("size=%d\n", size);
				sprintf(size_buf, "%d", size);
				write(cam_cli, size_buf, 10);
				int n = write(cam_cli, jpeg_buffer, size);
				printf("size=%d\n", n);
				
				if(n < 0)
				{
					close(cam_cli);
					printf("camera client close!\n");
					break;
				}
			}
		}
		close(cam_soc);
	}

/***************************************父进程*******************************************/

	//创建控制套接字
	int ctl_soc = socket(AF_INET, SOCK_STREAM, 0);
	if(ctl_soc < 0)
	{
		perror("socket");
		exit(-1);
	}

	int on = 1;
	setsockopt(ctl_soc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));//设置ip重用

	//绑定
	struct sockaddr_in ctl_addr;
	bzero(&ctl_addr, sizeof(ctl_addr));
	ctl_addr.sin_family = AF_INET;
	ctl_addr.sin_port = htons(8888); //控制端口
	ctl_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("192.168.2.234");
	if(bind(ctl_soc, (struct sockaddr *)&ctl_addr, sizeof(ctl_addr)) <0 )
	{
		perror("bind");
		exit(-1);
	}

	//监听
	listen(ctl_soc, 10);
	signal(SIGPIPE, foo);

	
	//接受并处理
	struct fdset Ctlfd;

	pthread_mutex_init(&mutex, NULL);

	while(1)
	{
		printf("wait client.\n");
		int msg_fd = accept(ctl_soc, NULL, NULL);

		memset(&Ctlfd, 0, sizeof(Ctlfd));
		Ctlfd.ctlsoc = msg_fd;
		Ctlfd.portfd = fd;

		if(0 != pthread_create(&tid1, NULL,Ctl_request, (void *)(&Ctlfd)))//处理控制
		{
			perror("pthread_create");
		}

		if(0 != pthread_create(&tid2, NULL,Send_Msg, (void *)(&Ctlfd)))//发送信息
		{
			perror("pthread_create");
		}
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);
		close(msg_fd);
		close(fd);
	}
	close(ctl_soc);
	return 0;
}


unsigned char Get_temp(unsigned char * buf)
{
	unsigned char temp_val = buf[4] + buf[5];
	return temp_val;
}

unsigned char Get_hum(unsigned char * buf)
{
	unsigned char hum_val = buf[6] + buf[7];
	return hum_val;
}

unsigned short Get_illuminate(unsigned char * buf)
{
	unsigned short illum_val = buf[20] + buf[21] + buf[22] + buf[23];
	return illum_val;
}

void Ctl_dev(int portfd, char *cmd)
{
	if(strncmp(cmd, "ledon", 5) == 0)
	{
		write(portfd, led_on, sizeof(led_on));
	}
	else if(strncmp(cmd, "ledoff", 6) == 0)
	{
		write(portfd, led_off, sizeof(led_off));
	}
	else if(strncmp(cmd, "fanon", 5) == 0)
	{
		write(portfd, fan_on, sizeof(fan_on));
	}
	else if(strncmp(cmd, "fanoff", 6) == 0)
	{
		write(portfd, fan_off, sizeof(fan_off));
	}
	else if(strncmp(cmd, "beepon", 6) == 0)
	{
		write(portfd, beep_on, sizeof(beep_on));
	}
	else if(strncmp(cmd, "beepoff", 7) == 0)
	{
		write(portfd, beep_off, sizeof(beep_off));
	}
}

void * Ctl_request(void *arg)
{
	struct fdset *cfd = (struct fdset *)arg;
	int n = 0;
	char cmd[10] = {0};//命令缓存
	while(1)
	{
		memset(cmd, 0, sizeof(cmd));
		n = read(cfd->ctlsoc, cmd, sizeof(cmd));
		printf("%s\n", cmd);
		pthread_mutex_lock(&mutex);
		Ctl_dev(cfd->portfd, cmd);
		pthread_mutex_unlock(&mutex);
	}
}

void * Send_Msg(void *arg)
{
	unsigned char temp = 0;
	unsigned char hum  = 0;
	unsigned int illum = 0;

	unsigned char buf[36] = {0};//接收串口信息
	unsigned char temperature[3] = {0};
	unsigned char humidity[3] = {0};
	unsigned char illuminate[5] = {0};
	unsigned char Data_buf[14] = {0};

	struct fdset *cfd = (struct fdset *)arg;
	int n = 0;

	while(1)
	{

reget:
		usleep(100);
		memset(buf, 0, sizeof(buf));
		memset(temperature, 0, sizeof(temperature));
		memset(humidity, 0, sizeof(humidity));
		memset(illuminate, 0, sizeof(illuminate));

		pthread_mutex_lock(&mutex);
		n = read(cfd->portfd, buf, sizeof(buf));//读取串口数据
		pthread_mutex_unlock(&mutex);

		temp = Get_temp(buf);
		hum  = Get_hum(buf);
		illum = Get_illuminate(buf);
		if((temp == 0) && (hum == 0) && (illum == 0))
			goto reget;

		sprintf(temperature, "%u", temp);
		sprintf(humidity, "%u", hum);
		sprintf(illuminate, "%u", illum);

		memset(Data_buf, 0, sizeof(Data_buf));
		Data_com(Data_buf, temperature, humidity, illuminate);
		n = write(cfd->ctlsoc, Data_buf, sizeof(Data_buf));
		if(n < 0)
		{
			pthread_cancel(tid1);
			break;
		}
		printf("%s\n",Data_buf);
	}
}


void Data_com(unsigned char* Data_buf, unsigned char* temperature, unsigned char* humidity, unsigned char* illuminate)
{
	strcat(Data_buf, temperature);
	strcat(Data_buf, ",");
	strcat(Data_buf, humidity);
	strcat(Data_buf, ",");
	strcat(Data_buf, illuminate);
}





#include "camera.h"


int Open_Cam(char *dev)
{
	//打开摄像头
	if((fd = open(dev, O_RDWR)) == -1) 
	{
		printf("Open camera error!\n");
		return -1;
	}

	
	//查看设备属性
	if(ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) 
	{
		printf("%s: Can't query dev.\n",FILE_VIDEO);
		return -1;
	}
	else
	{
     	printf("driver:\t\t%s\n",cap.driver);
     	printf("card:\t\t%s\n",cap.card);
     	printf("bus_info:\t%s\n",cap.bus_info);
     	printf("version:\t%d\n",cap.version);
     	printf("capabilities:\t%x\n",cap.capabilities);
     	
     	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE) 
     	{
			printf("Device %s: supports capture.\n",FILE_VIDEO);
		}

		if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING) 
		{
			printf("Device %s: supports streaming.\n",FILE_VIDEO);
		}
	} 
	
    //设置格式
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.height = IMAGEHEIGHT;
	fmt.fmt.pix.width = IMAGEWIDTH;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	if(ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
	{
		printf("Unable to set format\n");
		return -1;
	} 	
	
	if(ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
	{
		printf("Unable to get format\n");
		return -1;
	}
 	printf("fmt.type:\t\t%d\n",fmt.type);
 	printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
 	printf("pix.height:\t\t%d\n",fmt.fmt.pix.height);
 	printf("pix.width:\t\t%d\n",fmt.fmt.pix.width);
 	printf("pix.field:\t\t%d\n",fmt.fmt.pix.field);
	
	//设置帧率
	fps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fps.parm.capture.timeperframe.denominator = 30;
	fps.parm.capture.timeperframe.numerator = 1;
	if(ioctl(fd, VIDIOC_S_PARM, &fps) == -1)
    {
        printf("Set frame rate error!\n");
        return -1;
    }
	
	printf("init %s \t[OK]\n",FILE_VIDEO);
	
	return fd;
}

int Init_Cam(int fd)
{
	unsigned int n_buffers;

	//申请缓存
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(ioctl(fd,VIDIOC_REQBUFS,&req) == -1)
	{
		printf("Request for buffers error\n");
		return -1;
	}

	//分配内存
	buffers = malloc(req.count*sizeof(*buffers));
	if(!buffers) 
	{
		printf("Malloc memory error!\n");
		return -1;
	}

	//内存映射
	for(n_buffers = 0; n_buffers < req.count; n_buffers++) 
	{

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		//查询队列
		if(ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			printf("Query buffer error!\n");
			return -1;
		}

		//映射
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		
		if(buffers[n_buffers].start == MAP_FAILED)
		{
			printf("Buffer map error!\n");
			return -1;
		}
	}

	//开始采集图片
	for(n_buffers = 0; n_buffers < req.count; n_buffers++)
	{
		buf.index = n_buffers;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ioctl(fd, VIDIOC_QBUF, &buf);
	}
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ioctl(fd, VIDIOC_STREAMON, &type);
	convert_rgb_to_jpg_init();
	return 0;
}


int DQ_JPEG2Buf(int fd, void *picbuf)
{

	if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
	{
    	return -1;
	}
 	convert_yuv_to_rgb(buffers[buf.index].start, rgb_buffer, IMAGEWIDTH, IMAGEHEIGHT ,24);
	int size = convert_rgb_to_jpg_work(rgb_buffer, picbuf, IMAGEWIDTH, IMAGEHEIGHT ,24, 60);
	return size;

}

int Q_Pic2Buf(int fd)
{
	if(-1 == ioctl(fd, VIDIOC_QBUF, &buf)) //入队
	{
		return -1;
	}
	return 0;
}


int Close_Cam(int fd)
{
	int i = 0;
	for(i = 0; i < req.count; i++)
	{
		munmap(buffers[i].start, buffers[i].length);
	}
    free(buffers);
    close(fd);
    convert_rgb_to_jpg_exit();
    return 0;
}




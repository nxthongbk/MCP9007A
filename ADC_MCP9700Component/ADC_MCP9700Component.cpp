#include "legato.h"
#include "interfaces.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "util/paint.h"
#include "util/images.h"


#define COLORED      0
#define UNCOLORED    1

#define ADC_SAMPLE_INTERVAL_IN_MILLISECONDS (1000)

unsigned char* frame_buffer;
Paint paint(frame_buffer, 0, 0);
unsigned char *fbp;

int screensize, row_size;
int width,height;


static void DisplayScreen(unsigned char *fbData, unsigned char *imageData, int screenSize)
{
	for(int cnt = 0;cnt < screenSize;cnt++)	{
					*(fbData+cnt)=*(imageData+cnt);
			}
}

//--------------------------------------------------------------------------------------------------
/**
* Timer handler  will publish the current ADC reading.
 */
//--------------------------------------------------------------------------------------------------
static void adcTimer
(
    le_timer_Ref_t adcTimerRef
)
{
    int32_t value;
    float temperature;
    char  temp[10];

    const le_result_t result = le_adc_ReadValue("EXT_ADC0", &value);

    if (result == LE_OK)
    {
        temperature = (value *1.8)/1800;
        temperature = temperature- 0.5;
        temperature = temperature/0.01;
        printf("temperature is: %2.2f\n",temperature);

        Paint paint(frame_buffer, 0, 0);
        paint.SetWidth(128);
        paint.SetHeight(250);

        paint.Clear(UNCOLORED);
        DisplayScreen(fbp, frame_buffer, screensize);

		sprintf(temp,"%2.2f",temperature);

		paint.Clear(0, 60, 128,80, UNCOLORED);
		paint.DrawStringAt(18, 60, "TEMPER:", &Font16, COLORED);

		paint.Clear(0,180,128,204, UNCOLORED);
		paint.DrawStringAt(20, 100, temp, &Font24, COLORED);
		DisplayScreen(fbp, frame_buffer, screensize);
    }
    else
    {
        LE_INFO("Couldn't get ADC value");
    }
}


COMPONENT_INIT
{
	int fd;
	struct fb_var_screeninfo vinfo;

	fd = open("/dev/fb0", O_RDWR);
	if (fd == -1) {
		LE_INFO("Error: cannot open framebuffer device");
		exit(0);
	}

	if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)== -1) {
		perror("Error reading variable information");
		exit (-1);
	}

	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel /8;

	width = vinfo.xres;
	height= vinfo.yres * vinfo.bits_per_pixel /8;

	LE_INFO("screen size is:%d",screensize);


	fbp = (unsigned char *)mmap(0,screensize,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);

	if((int)fbp == -1)
	{
		exit(4);
		LE_INFO("Fail to map FrameBuffer device to memory");
	}
	LE_INFO("The FrameBuffer device was mapped to memory successfully\n");

	frame_buffer = (unsigned char*)malloc(width * height);


	//LE_INFO("---------------------- ADC Reading started");
	le_timer_Ref_t adcTimerRef = le_timer_Create("ADC Timer");
	le_timer_SetMsInterval(adcTimerRef, ADC_SAMPLE_INTERVAL_IN_MILLISECONDS);
	le_timer_SetRepeat(adcTimerRef, 0);
	le_timer_SetHandler(adcTimerRef, adcTimer);
	le_timer_Start(adcTimerRef);
}

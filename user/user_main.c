/*
	Read DS18B20 and broadcast the result via UDP

	This includes a very very stupid configuration routine if the wifi doesn't connect.
	Stupid, but currently effective.

	FYI I like the baud rate 74880 (bootloader rate, so i don't get garbage), change it as you wish

	Consider this thing pre-alpha in its current form

*/

#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <mem.h>
#include "driver/uart.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/ds18b20.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"

#define SLEEP_TIME 120 * 1000 /* milliseconds */
#define MAX_DEVICES 20

#define sleepms(x) os_delay_us(x*1000);
//#define DSDEBUG 1

uint8_t devices[MAX_DEVICES][8];
int ndevices = 0;
int curdevice = 0;
char *rline = NULL;
int rstate = 0;
char ap[50];
char pass[50];

static os_timer_t ds18b20_timer;


static struct espconn bcconn;


void ICACHE_FLASH_ATTR ds18b20p2(void *arg);

void ICACHE_FLASH_ATTR sleeper(void *arg)
{
	INFO("deep_sleep\n");
	system_deep_sleep((SLEEP_TIME-1000)*1000);
}


//  ds18b20 part 1
//  initiate a conversion
void ICACHE_FLASH_ATTR ds18b20p1(void *arg)
{
	os_timer_disarm(&ds18b20_timer);

	if (ndevices < 1)
	{
		INFO("no devices found\n");
		os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)sleeper, (void *)0);
		os_timer_arm(&ds18b20_timer, 1000, 0);
		return;
	}

	// perform the conversion
	reset();
	select(devices[curdevice]);
	write(DS1820_CONVERT_T, 1); // perform temperature conversion

	// tell me when its been 750ms, please
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)ds18b20p2, (void *)0);
	os_timer_arm(&ds18b20_timer, 750, 0);

}

// ds18b20 part 2
// conversion should be done, get the result
// report it
// check for next device, call part 1 again
// or sleep if done
void ICACHE_FLASH_ATTR ds18b20p2(void *arg)
{
	int i;
	int tries = 5;
	uint8_t data[12];
	os_timer_disarm(&ds18b20_timer);
	while(tries > 0)
	{
#ifdef DSDEBUG
		INFO("Scratchpad: ");
#endif
		reset();
		select(devices[curdevice]);
		write(DS1820_READ_SCRATCHPAD, 0); // read scratchpad

		for(i = 0; i < 9; i++)
		{
			data[i] = read();
#ifdef DSDEBUG
			INFO("%02x ", data[i]);
#endif
		}
#ifdef DSDEBUG
		INFO("\n");
		INFO("crc calc=%02x read=%02x\n",crc8(data,8),data[8]);
#endif
		if(crc8(data,8) == data[8]) break;
		tries--;
	}
	uint8_t *addr = devices[curdevice];
	int rr = data[1] << 8 | data[0];
	if (rr & 0x8000) rr |= 0xffff0000; // sign extend
	if (addr[0] == 0x10)
	{
		//DS18S20
		rr = rr * 10000 / 2; // each bit is 1/2th of a degree C, * 10000 just keeps us as an integer
	}
	else
	{
		//DS18B20
		rr = rr * 10000 / 16; // each bit is 1/16th of a degree C, * 10000 just keeps us as an integer
	}
#ifdef DSDEBUG
	INFO("int reading=%d r2=%d.%04d hex=%02x%02x\n",rr,rr/10000,abs(rr)%10000,data[1],data[0]);
#endif
	char out[50];
	os_sprintf(out,"%02x%02x%02x%02x%02x%02x%02x%02x:%d.%04d",
					addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7],
					rr/10000,abs(rr)%10000);

	INFO("%s\n",out);

	// setup for next device
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)ds18b20p1, (void *)0);
	os_timer_arm(&ds18b20_timer, 100, 0);

	curdevice++;
	if (curdevice >= ndevices)
	{
		curdevice = 0;
		// no more devices, go to sleep...
		// ... but after a second (ensures the last UDP packet is sent before shutdown)
		os_timer_disarm(&ds18b20_timer);
		os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)sleeper, (void *)0);
		os_timer_arm(&ds18b20_timer, 1000, 0);
	}
}

// init!
void ICACHE_FLASH_ATTR user_done(void)
{

	INFO("started\n");
	INFO("press return to configure wifi.\n");
	INFO("unless you've configured it once already.\n");
	
    INFO("ready\n");
	struct ip_info ipconfig;

    // init 18b20 driver
	ds_init();

	ndevices = 0;
	int r;
	uint8_t addr[8];

	// find the 18b20s on the bus
	while( (r = ds_search(addr)) )
	{
		if(crc8(addr, 7) != addr[7]) continue; // crc mismatch -- bad device
		INFO("Found:%02x%02x%02x%02x%02x%02x%02x%02x\n",
				addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
		if (addr[0] == 0x10 || addr[0] == 0x28)
		{
			memcpy(devices[ndevices],addr,8);
			ndevices++;
			if (ndevices >= MAX_DEVICES) break;
		}
	}

	// start reading devices...
	// ... i'm handing off with a timer, could have called directly
	os_timer_disarm(&ds18b20_timer);
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)ds18b20p1, (void *)0);
	os_timer_arm(&ds18b20_timer, 100, 0);
}
void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200); 
    system_init_done_cb(user_done);
    INFO("\r\nSystem started ...\r\n");

}

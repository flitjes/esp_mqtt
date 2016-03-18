/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
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
#include "ws2812.h"

MQTT_Client mqttClient;
#define LEDS 8
//GRB
char buffer1[LEDS*3] = { 
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF,
		       }; 
//GRB
char buffer2[LEDS*3] = { 
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
			0x00, 0xFF, 0x00,
		       };

uint8_t toggle = 0;

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}
void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	/*Pick a unique topic*/
	MQTT_Subscribe(client, "/AI-lab/workshop/grp0/toggle", 0);

	/*Pick a unique topic*/
	char* publish= "MQTT initialised";
	MQTT_Publish(client, "/AI-lab/workshop/grp0/status",publish , strlen(publish), 0, 0);

}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	if(toggle == 0){
		os_printf("White\r\n");
		
		toggle = 1;
	} else if(toggle == 1){
		os_printf("Red\r\n");
		
		toggle = 0;
	}
	os_free(topicBuf);
	os_free(dataBuf);
}


void user_done(void)
{
	CFG_Load();
	/*Connect to wifi*/
	/*Init Conneciton*/
	/*Init Client*/
	/*Init LWT*/
	/*Register connection callback*/
	/*Register disconnect callback*/
	/*Register publish callback*/
	/*Register subcribe callback*/
	

}

void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200); 
    system_init_done_cb(user_done);
    INFO("\r\nSystem started ...\r\n");

}

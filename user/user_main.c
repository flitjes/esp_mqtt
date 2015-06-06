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
#include "jsmn.h"
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

MQTT_Client mqttClient;

#define CONNECTED 1
#define DISCONNECTED 0

#define SIG_UART0_RX 0x7f

struct {
    int wifi;
    int mqtt;
} typedef status;

status st;

char* get_value(char * input, int start, int end){
    int size = end - start;
    char* value = (char*)os_malloc(sizeof(char) * (size + 1));
    memcpy(value, &input[start], (end - start));
    value[size] = '\0';
//    os_printf("%s\r\n", value);
    return value;
}
struct {
    char host[17];
    char port[8];
    char clientid[50];
    char user[20];
    char password[50];
    char interval[8];
} typedef esp_init_cfg;

esp_init_cfg esp_init;

struct {
    char bssid[50];
    char psk[50];
} typedef wifi_cfg;

wifi_cfg wifi;

struct {
char* key;
char* value;
} typedef keyvalue;

struct {
    char host[17];
    char topic[255];
} typedef subscribe;

subscribe sub;

struct {
    char host[17];
    char topic[255];
    char message[255];
} typedef publish;

publish pub;

MQTT_Client* client;

void setup_mqtt(esp_init_cfg* esp_cfg);

int get_key_value(char* input, jsmntok_t tok[], int index, const char* key_value, keyvalue* kv){
    int start =tok[index].start;
    int end = tok[index].end;
    int size = end - start;
    char* key = get_value(input, start, end);
    int retval = 1;
    
    if(!strcmp(key, key_value)){ 
        char* value = get_value(input, tok[index+1].start, tok[index+1].end);
    //    os_printf("%s = %s\r\n", key, value);

        kv-> key = (char*) os_malloc (strlen(key) + 1);
        strcpy(kv->key, key);
        kv-> value = (char*) os_malloc (strlen(value) + 1);
        strcpy(kv->value, value);
        retval = 0;
        os_free(value);
     }
     os_free(key);
    return retval;
}

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
        st.wifi = CONNECTED;
	} else {
		MQTT_Disconnect(&mqttClient);
        st.wifi = DISCONNECTED;
	}
}
void mqttConnectedCb(uint32_t *args)
{
	client = (MQTT_Client*)args;
    st.mqtt = CONNECTED;
	INFO("MQTT: Connected\r\n");

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

	INFO("Receive topic: %s, data:\n%s\n", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

void setup_mqtt(esp_init_cfg* esp_cfg){

	MQTT_InitConnection(&mqttClient, esp_cfg->host, atoi(esp_cfg->port), 0);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
}

void process_rx_input(char *input, int length);
int sub_count = 0;
char rxbuffer[256];
int buf_index = 0;
os_event_t uart_recv_queue[32];
void uart_recv_task(os_event_t * event)
{
	switch(event->sig)
	{
		case SIG_UART0_RX:
		{
			os_memcpy(&rxbuffer[buf_index], &event->par,1);
			os_printf("%c", rxbuffer[buf_index]);
			rxbuffer[buf_index + 1] = '\0';
			if(rxbuffer[buf_index] == '*'){
				process_rx_input(rxbuffer, buf_index);			
				buf_index = 0;
			}
			else {
				if(buf_index + 1 < 254)
					buf_index++;
				else
					buf_index = 0;
			}
			
			break;
		}
		default:
		break;
	}
}
void process_rx_input(char *input, int length){
    jsmn_parser p;
    jsmntok_t tok[50];
    const char* js;
    int i = 0;    
    int count;
    keyvalue kv;
    
    INFO(input);
    INFO("\r\n");
    
    js = input;
    jsmn_init(&p);
    count = jsmn_parse(&p, js, strlen(js), tok, 50);
    os_printf("Count: %d\r\n", count); 
    if(count > 1){
        keyvalue first;
        for(i=0; i < count; i++){
            unsigned int start =tok[i].start;
            unsigned int end = tok[i].end;
        
            unsigned int size = 0;
            if(end > 0)
                size = end - start;

            //os_printf("Start %d, end %d size %d\r\n", start ,end, size);
        
            if(start >= 0 && end >= 0 && size > 0){
                if(!get_key_value(input, tok, 1, "subscribe", &first)){
                    if(!strcmp(first.key, "subscribe")){
                        if(!get_key_value(input, tok, i, "host", &kv))
                            memcpy(sub.host, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "topic", &kv))
                            memcpy(sub.topic, kv.value, strlen(kv.value));
                    }
                }
                else 
                if(!get_key_value(input, tok, 1, "publish", &first)){
                    if(!strcmp(first.key, "publish")){
                        if(!get_key_value(input, tok, i, "host", &kv))
                            memcpy(pub.host, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "topic", &kv))
                            memcpy(pub.topic, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "message", &kv))
                            memcpy(pub.message, kv.value, strlen(kv.value));
                    }
                }
                else
                if(!get_key_value(input, tok, 1, "wifi", &first)){
                    if(!strcmp(first.key, "wifi")){
                        if(!get_key_value(input, tok, i, "bssid", &kv))
                            memcpy(wifi.bssid, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "psk", &kv))
                            memcpy(wifi.psk, kv.value, strlen(kv.value));
                    }
                }
                else
                if(!get_key_value(input, tok, 1, "init", &first)){
                    if(!strcmp(first.key, "init")){
                        if(!get_key_value(input, tok, i, "host", &kv))
                            memcpy(esp_init.host, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "port", &kv))
                            memcpy(esp_init.port, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "clientid", &kv))
                            memcpy(esp_init.clientid, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "user", &kv))
                            memcpy(esp_init.user, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "pass", &kv))
                            memcpy(esp_init.password, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "interval", &kv))
                            memcpy(esp_init.interval, kv.value, strlen(kv.value));
                    }
                }
            }
        }
    
    }
    
    if(esp_init.host[0] != '\0' && st.mqtt != CONNECTED){
        os_printf("Initialising %s, %s, %s, %s, %s, %s\n", esp_init.host, esp_init.port, esp_init.clientid, esp_init.user, esp_init.password, esp_init.interval);
        setup_mqtt(&esp_init);
	MQTT_Connect(&mqttClient);
	os_printf("Inited\n");
    }

    if(sub.host[0] != '\0'){
        os_printf("Subscribe to %s, %s\n", sub.host, sub.topic);
	    MQTT_Subscribe(client, sub.topic, sub_count);
        sub_count++;
        //Ugly but it works for now
        sub.host[0] = '\0';
    }
    
    if(pub.host[0] != '\0'){
        os_printf("Publish to %s, %s, %s\n", pub.host, pub.topic, pub.message);
        MQTT_Publish(client, pub.topic, pub.message, strlen(pub.message), 0, 0);
        //Ugly but it works for now
        pub.host[0] = '\0';
    }

    if(wifi.bssid[0] != '\0' && st.wifi != CONNECTED){
        os_printf("Wifi: %s, %s\n", wifi.bssid, wifi.psk);
	    WIFI_Connect(wifi.bssid, wifi.psk, wifiConnectCb);
    }

}
void user_init(void)
{
    esp_init.host[0] = '\0';
    esp_init.port[0] = '\0';
    esp_init.clientid[0] = '\0';
    esp_init.user[0] = '\0';
    esp_init.password[0] = '\0';
    esp_init.interval[0] = '\0';

    sub.host[0] = '\0';
    sub.topic[0] = '\0';

    pub.host[0] = '\0';
    pub.topic[0] = '\0';
    pub.message[0] = '\0';

    wifi.bssid[0] = '\0';
    wifi.psk[0] = '\0';

    st.wifi = DISCONNECTED;
    st.mqtt = DISCONNECTED;

    uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	CFG_Load();

	INFO("\r\nSystem started ...\r\n");
	// install receive task
	system_os_task(uart_recv_task, UART_TASK_PRIO, uart_recv_queue, 32);
}

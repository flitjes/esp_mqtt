#include "jsmn.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* input_list[] ={ 
                "{\"init\":{  \"host\":\"192.168.178.x\", \"port\":\"1883\", \"clientid\":\"esp123\", \"user\":\"username\", \"pass\":\"password\", \"interval\":\"120\" }}\n",
                "{\"subscribe\":{  \"host\":\"192.168.178.x\", \"topic\":\"/abc/de/fgh\"}}\n",
                "{\"publish\":{  \"host\":\"192.168.178.x\", \"topic\":\"/abc/de/fgh\", \"message\":\"Hello World\"}}\n",
                "{\"wifi\":{  \"bssid\":\"wifi 123\", \"psk\":\"yourpsk\"}}\n",
                };

char* get_value(char * input, int start, int end){
    int size = end - start;
    char* value = (char*)malloc(sizeof(char) * (size + 1));
    memcpy(value, &input[start], (end - start));
    value[size] = '\0';
//    printf("%s\n", value);
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

int get_key_value(char* input, jsmntok_t tok[], int index, const char* key_value, keyvalue* kv){
    int start =tok[index].start;
    int end = tok[index].end;
    int size = end - start;
    char* key = get_value(input, start, end);
    int retval = 1;
    if(!strcmp(key, key_value)){ 
        char* value = get_value(input, tok[index+1].start, tok[index+1].end);
//        printf("%s = %s\n", key, value);

        kv-> key = (char*) malloc (strlen(key) + 1);
        strcpy(kv->key, key);
        kv-> value = (char*) malloc (strlen(value) + 1);
        strcpy(kv->value, value);
        retval = 0;
        free(value);
     }
     free(key);
    return retval;
}

void main(void){
    jsmn_parser p;
    jsmntok_t tok[50];
    const char* js;
    int i = 0;    
    int j,k;
    int count;
    keyvalue kv;
    
    for(k = 0; k < 4; k++){

    char* input = input_list[k];
//    printf("%s", input);
    js = input;
    jsmn_init(&p);
    count = jsmn_parse(&p, js, strlen(js), tok, 50);
 
    if(count > 0){
        keyvalue first;
        for(i=0; i < count; i++){
            int start =tok[i].start;
            int end = tok[i].end;
            int size = end - start;
            
            if(start >= 0 && end >= 0 && size > 0){
                if(!get_key_value(input, tok, 1, "init", &first));
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
                if(!get_key_value(input, tok, 1, "subscribe", &first))
                    if(!strcmp(first.key, "subscribe")){
                        if(!get_key_value(input, tok, i, "host", &kv))
                            memcpy(sub.host, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "topic", &kv))
                            memcpy(sub.topic, kv.value, strlen(kv.value));
                    }
        
                if(!get_key_value(input, tok, 1, "publish", &first))
                    if(!strcmp(first.key, "publish")){
                        if(!get_key_value(input, tok, i, "host", &kv))
                            memcpy(pub.host, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "topic", &kv))
                            memcpy(pub.topic, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "message", &kv))
                            memcpy(pub.message, kv.value, strlen(kv.value));
                    }
        
                if(!get_key_value(input, tok, 1, "wifi", &first))
                    if(!strcmp(first.key, "wifi")){
                        if(!get_key_value(input, tok, i, "bssid", &kv))
                            memcpy(wifi.bssid, kv.value, strlen(kv.value));
                        if(!get_key_value(input, tok, i, "psk", &kv))
                            memcpy(wifi.psk, kv.value, strlen(kv.value));
                    }
            }
        }
    
    }
    }
    printf("Initialising %s, %s, %s, %s, %s, %s\n", esp_init.host, esp_init.port, esp_init.clientid, esp_init.user, esp_init.password, esp_init.interval);
    printf("Subscribe to %s, %s\n", sub.host, sub.topic);
    printf("Publish to %s, %s, %s\n", pub.host, pub.topic, pub.message);
    printf("Wifi: %s, %s\n", wifi.bssid, wifi.psk);
}       

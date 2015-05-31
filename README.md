**esp_mqtt**
==========
![](https://travis-ci.org/tuanpmt/esp_mqtt.svg?branch=master)

This is MQTT client library for ESP8266, port from: [MQTT client library for Contiki](https://github.com/esar/contiki-mqtt) (thanks)



**Features:**

 * Support subscribing, publishing, authentication, will messages, keep alive pings and all 3 QoS levels (it should be a fully functional client).
 * Support multiple connection (to multiple hosts).
 * Support SSL connection (max 1024 bit key size)
 * Easy to setup and use
 * Json interface to connect, subscribe and publish
 

**Json interface:**
Each line should end with "\r\n" This is part of the parsing which is done on a new line including the carriage return.

MQTT init:
{"init": {"host": "192.168.178.x", "port": "1883", "clientid": "esp123", "user": "username", "pass": "password",  "interval": "120"}}

Wifi init:
{"wifi": {"bssid": "yourbssid", "psk": "yourpsk"}}

Subscribe:
{"subscribe": {"host": "192.168.178.x","topic": "/abc/de/fgh"}}

Publish:
{"publish": {"host": "192.168.178.x", "topic": "/test", "message": "Hello World"}}


**LICENSE - "MIT License"**

Copyright (c) 2014-2015 Tuan PM, https://twitter.com/TuanPMT

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

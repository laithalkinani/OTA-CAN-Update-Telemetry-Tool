/*
wifi_stuff.h
Purpose: TCP client in esp32
Author: Laith Al-Kinani
*/

#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#define HOST_IP "172.22.134.141"
#define PORT    1309 //figure this out later

static const char* WIFI_TAG = "WIFI_STUFF";

esp_err_t createClient(void);
void tcp_client(void *arg);





#endif //WIFI_STUFF_H
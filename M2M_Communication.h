//This is the header that is going to be called in the main code block like a library
//It is where funciton names should be defined and will refrence M2M_Communication.cpp
#ifndef ESPNOWCOMMS_H
#define ESPNOWCOMMS_H

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Message structure
typedef struct struct_message {
    uint8_t sender_ID;
    int motor_1;
    int motor_2;
    int motor_3;
    int motor_4;
    uint32_t timestamp;
} struct_message;

// Global state arrays
extern struct_message deviceState[20];
extern bool deviceOnline[20];
extern unsigned long lastUpdate[20];

// Main interface functions
int nowSetup();

void nowUpdate(
    int myID,
    int motor_1,
    int motor_2,
    int motor_3,
    int motor_4,
    int send_IDS[],
    int send_count
);

void checkOnlineStatus();

// Helper functions
bool readMacAddress(uint8_t mac[6]);

int MACtoID(
    const uint8_t MAC[6],
    const uint8_t devices[][6],
    int numDevices
);

// ESP-NOW callbacks
void OnDataSent(
    const uint8_t *mac_addr,
    esp_now_send_status_t status
);

void OnDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
);

#endif
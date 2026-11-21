//This is the header that is going to be called in the main code block like a library
//It is where funciton names should be defined and will refrence M2M_Communication.cpp
#ifndef ESPNOWCOMMS_H
#define ESPNOWCOMMS_H

#include <Arduino.h>
#include <esp_now.h>

typedef struct
{
    int motor_1;
    int motor_2;
    int motor_3;
    int motor_4;
} struct_message;

void nowSetup();

void nowUpdate(
    int motor_1,
    int motor_2,
    int motor_3,
    int motor_4,
    int send_IDS[],
    int send_count
);

String readMacAddress();

int MACtoID(
    const String& MAC,
    const String devices[],
    int numDevices
);

String IDtoMAC(
    int ID,
    const String devices[],
    int numDevices
);

bool MACcol_to_MACarr(
    const String& MACcol,
    uint8_t MACarr[6]
);

void OnDataSent(
    const uint8_t *mac_addr,
    esp_now_send_status_t status
);

#endif
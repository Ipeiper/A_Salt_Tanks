#include "ESPNowComms.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

//This is where the actions of the funcitons happpens.
//This file should be exclusively fuctions, no startup or loop
//All funcitons here should be called by their name in the .h file of the same name

//Make sure to add this to the main file
/*
#include <WiFi.h>
#include <esp_wifi.h>
*/

//defines type of struct for messages
  typedef struct struct_message{
    int motor_1;
    int motor_2;
    int motor_3;
    int motor_4;
  } struct_message;
  struct_message myData;

//runs the setup code for esp now and establishes variables like setup uaually does
//should hold that a list that will be hardcoded for the MAC address and the device names of each. Maybe make it pull from a file called something like DeviceMacRegistry that is a CSV with the macs and device ids
//needs to find the devices MAC address, establish which device that means it is, and start communicating to the others
void nowSetup(){
  //Variables that define devices and device ids. device ID is automatically assigned as the index in the array of the MAC
  uint8_t devices[]={{0x70,0x4B,0xCA,0x25,0xC1,0x5C},};
  String myMAC=readMacAddress();
  int myID=MACtoID(myMAC,devices);
  if (WIFI.getMode()== WIFI_OFF){
    WIFI.mode(WIFI_AP_STA);
  }
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_peer_info_t peerInfo;
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  
  for (p = 0; p<sizeof(devices)/sizeof(devices[0]);p++){
    if(p!=myID){
      //register peer
      memcpy(peerInfo.peer_addr, MACcol_to_MACarr(devices[p]), 6);
      peerInfo.channel = 0; 
      peerInfo.encrypt = false;
      //add peer
      if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
  }
    }
    
  }

}

//checks to see if there are any new messages.
//should store the previous message in storage and if there is nothing new out it should send out the previous values
//Currently String, subject to change
//Need to figure out what format is going to work best, it should probably output a String with the values from the protocol and let another module do the actaul processing

//motor 1 is, motor 2 is, motor 3 is, motor 4 is, send_devs is an array of device IDs that are getting the message sent to them
String nowUpdate(int motor_1,
  int motor_2,
  int motor_3,
  int motor_4,
  int send_IDS,
  int send_count){
  myData.motor_1=motor_1;
  myData.motor_2=motor_2;
  myData.motor_3=motor_3;
  myData.motor_4=motor_4;
  uint8_t send_MAC;
  for (i = 0; i<send_count;i++){
    //send message
    send_MAC = MACcol_to_MACarr(IDtoMAC(send_IDS[i]));
    esp_err_t result = esp_now_send(send_MAC, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
    Serial.print("Sent with success to:");
    Serial.println(send_IDS[i]);
  }
  else {
    Serial.print("Error sending the data");
    Serial.print(send_IDS[i]);
  }

  }
}
//(Optional) have a function that runs evey once in a whilethat finds all the nearby devices and only sends out pings to them rather than the whole fleet


//Helper functions

String readMacAddress(){
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    String MAC =
    String(baseMac[0], HEX) + ":" +
    String(baseMac[1], HEX) + ":" +
    String(baseMac[2], HEX) + ":" +
    String(baseMac[3], HEX) + ":" +
    String(baseMac[4], HEX) + ":" +
    String(baseMac[5], HEX);
    Serial.printf(MAC);
    return MAC;
  } else {
    Serial.println("Failed to read MAC address");
  }
}
//takes the mac address and converts it into a device ID. may return UNKNOWN if the MAC is unregistered
String MACtoID(MAC,devices){
  int ID;
  int IDfound =0;
  for (i = 0; i< sizeof(devices)/sizeof(devices[0]);i++){
        if (devices[i] == MAC){
            ID = i;
            IDfound=1;
            break;
        }
    }
    if (IDfound != 1){
      ID = "UNKNOWN";
    }

    return ID;
}
//takes a device ID and converts it to the MAC address, may return UNKNOWN if the ID is unregistered
String IDtoMAC(ID,devices){
  if 0<=ID && ID <sizeof(devices)/sizeof(devices[0])(devices) {
    MAC = devices[ID];
  }
  else{
    MAC = "UNKNOWN";
  }
  return MAC;
}
//converts the typical String colon spaced mac address into the uint8 array used for ESPnow
bool MACcol_to_MACarr(MACcol){
  int result = sscanf(MACcol.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:",&byte_1,&byte_2,&byte_3,&byte_4,&byte_5,&byte_6);
  uint8_t MACarr[6] = {byte_1,byte_2,byte_3,byte_4,byte_5,byte_6};
  return MACarr;

}
//Callback function
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

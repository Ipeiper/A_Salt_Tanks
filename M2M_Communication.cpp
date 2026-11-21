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
    uint8_t sender_ID;
    int motor_1;
    int motor_2;
    int motor_3;
    int motor_4;
    uint32_t timestamp;
  } struct_message;
  struct_message myData;

// defines the devices and their IDs
static uint8_t devices[][6] = {
    {0x70,0x4B,0xCA,0x25,0xC1,0x5C}
};
static int numDevices= sizeof(devices)/sizeof(devices[0]);

//array storing the most recent message from each possible device
struct_message deviceState[20];
//bool describing whether a given device is online
bool deviceOnline[20];
//when that device was last updated
unsigned long lastUpdate[20];
//runs the setup code for esp now and establishes variables like setup uaually does
//should hold that a list that will be hardcoded for the MAC address and the device names of each. Maybe make it pull from a file called something like DeviceMacRegistry that is a CSV with the macs and device ids
//needs to find the devices MAC address, establish which device that means it is, and start communicating to the others
//returns the devices ID
int nowSetup(){
  //Variables that define devices and device ids. device ID is automatically assigned as the index in the array of the MAC
  //enables WIFI
  if (WiFi.getMode()== WIFI_OFF){
    WiFi.mode(WIFI_STA);
  }
  // finds mac address
  uint8_t myMAC[6];
  if (!readMacAddress(myMAC))
  {
    Serial.println("Failed to read MAC");
    return -1;
  }
  int myID=MACtoID(myMAC,devices,numDevices);
  if (myID == -1){
    Serial.println("Failed to Find Own Device ID");
    return -1;
  }
  //sets self to online
  deviceOnline[myID] = true;
  lastUpdate[myID] = millis();
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return -1;
  }
  esp_now_peer_info_t peerInfo={};
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb((OnDataRecv));
  
  for (int p = 0; p<numDevices;p++){
    if(p!=myID){
      //register peer
      memcpy(peerInfo.peer_addr, devices[p], 6);
      peerInfo.channel = 0; 
      peerInfo.encrypt = false;
      //add peer
      if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
  }
    }
    
  }
  return myID;
}

//checks to see if there are any new messages.
//should store the previous message in storage and if there is nothing new out it should send out the previous values
//Currently String, subject to change
//Need to figure out what format is going to work best, it should probably output a String with the values from the protocol and let another module do the actaul processing

//motor 1 is, motor 2 is, motor 3 is, motor 4 is, send_devs is an array of device IDs that are getting the message sent to them
void nowUpdate(
  int myID,
  int motor_1,
  int motor_2,
  int motor_3,
  int motor_4,
  int send_IDS[],
  int send_count){
  myData.sender_ID=myID;
  myData.motor_1=motor_1;
  myData.motor_2=motor_2;
  myData.motor_3=motor_3;
  myData.motor_4=motor_4;
  myData.timestamp=millis();

  //loop that sends messages
  for (int i = 0; i<send_count;i++){
    //send message
    esp_err_t result = esp_now_send(devices[send_IDS[i]], (uint8_t *) &myData, sizeof(myData));
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

void checkOnlineStatus(){
    for(int id=0; id<numDevices; id++)
    {
        if(millis() - lastUpdate[id] > 5000)
        {
            deviceOnline[id] = false;
        }
    }
}
//(Optional) have a function that runs evey once in a whilethat finds all the nearby devices and only sends out pings to them rather than the whole fleet


//Helper functions

bool readMacAddress(uint8_t mac[6])
{
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);

    if (ret == ESP_OK)
    {
        return true;
    }

    return false;
}
//takes the mac address and converts it into a device ID. may return -1 if the MAC is unregistered
int MACtoID(const uint8_t MAC[6], const uint8_t devices[][6], int numDevices){
  int ID;
  int IDfound =0;
  for (int i = 0; i< numDevices;i++){
        if (memcmp(devices[i], MAC, 6) == 0){
            ID = i;
            IDfound=1;
            break;
        }
    }
    if (IDfound != 1){
      ID = -1;
    }

    return ID;
}
//Callback function
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void OnDataRecv(
    const uint8_t * mac,
    const uint8_t *incomingData,
    int len)
{
  struct_message msg;
  //checks to make sure that the length of the message is correct to prevent corruption
  if(len != sizeof(struct_message))
{
    return;
}

  memcpy(
        &msg,
        incomingData,
        sizeof(msg)
    );
    if(msg.sender_ID >= 20)
        return;
  //sets the devices state to the contents of the message
  deviceState[msg.sender_ID] = msg;  
  //sets the device to online
  deviceOnline[msg.sender_ID] = true;
  //sets the latest update to the current time stamp
  lastUpdate[msg.sender_ID] = millis();
    
}


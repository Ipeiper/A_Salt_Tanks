//This is where the actions of the funcitons happpens.
//This file should be exclusively fuctions, no startup or loop
//All funcitons here should be called by their name in the .h file of the same name

//Make sure to add this to the main file
/*
#include <WiFi.h>
#include <esp_wifi.h>
*/


//runs the setup code for esp now and establishes variables like setup uaually does
//should hold that a list that will be hardcoded for the MAC address and the device names of each. Maybe make it pull from a file called something like DeviceMacRegistry that is a CSV with the macs and device ids
//needs to find the devices MAC address, establish which device that means it is, and start communicating to the others
void nowSetup();{
    //Variables that define devices and device ids. device ID is automatically assigned as the index in the array of the MAC
    devices={"70:4b:ca:25:c1:5c",};
    myMAC=readMacAddress();
    for (i = 0; i< len(devices)){
        if devices[i] == myMAC;{
            myID= i;
        }

    }

}

//checks to see if there are any new messages.
//should store the previous message in storage and if there is nothing new out it should send out the previous values
//Currently string, subject to change
//Need to figure out what format is going to work best, it should probably output a string with the values from the protocol and let another module do the actaul processing


string nowUpdate();



//(Optional) have a function that runs evey once in a whilethat finds all the nearby devices and only sends out pings to them rather than the whole fleet


//Helper functions

string readMacAddress(){
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    MAC="%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]
    Serial.printf(MAC);
    return MAC
  } else {
    Serial.println("Failed to read MAC address");
  }
}
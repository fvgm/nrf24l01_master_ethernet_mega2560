// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _nrf24l01_master_ethernet_mega2560_H_
#define _nrf24l01_master_ethernet_mega2560_H_
#include "Arduino.h"
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Wire.h>

#include "libraries/ArduinoJson/ArduinoJson-v5.13.5.h"
#include "libraries/ds3231/RtcDS3231.h"
#include "libraries/nrf24L01/nRF24L01.h"
#include "libraries/nrf24L01/RF24_config.h"
#include "libraries/nrf24L01/RF24.h"

#define SD_CARD_CS_PIN 4
#define ETHERNET_CS_PIN 10
#define RF24_CE_PIN 7
#define RF24_CS_PIN 8


void CSpinsInitialize();
void RTCInitialize();
void SDCardInitialize();
void EthernetInitialize();
void RadioInitialize();

//end of add your includes here



//Do not add code below this line
#endif /* _nrf24l01_master_ethernet_mega2560_H_ */

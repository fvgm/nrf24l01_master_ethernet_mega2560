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
#include "libraries/ds3231/RtcDS3231.h"
#include "libraries/nrf24L01/nRF24L01.h"
#include "libraries/nrf24L01/RF24_config.h"
#include "libraries/nrf24L01/RF24.h"

//end of add your includes here


void printDirectory(File dir, int numTabs);
String formatTime(const RtcDateTime& dt, String format);
void StrClear(char *str, char length);
void receiveNodeData();

//Do not add code below this line
#endif /* _nrf24l01_master_ethernet_mega2560_H_ */

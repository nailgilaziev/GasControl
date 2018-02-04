#include "LoggerSD.hpp"
#include "Arduino.h"

#include <SPI.h>

bool LoggerSD::start() {
  dataFile = SD.open("console.log", FILE_WRITE);
  if (dataFile) {
    fileAvailableToWrite = true;
  } else {
    fileAvailableToWrite = false;
    Serial.println("Error while trying to log to SD card");
  }
  return fileAvailableToWrite;
}

void LoggerSD::stop() {
  if(fileAvailableToWrite)
    dataFile.close();
  fileAvailableToWrite = false;
}

void LoggerSD::print(const char cs[]) {}
void LoggerSD::println(const char cs[]) {}
void LoggerSD::println(int num, int base = DEC) {}
void LoggerSD::print(int num, int base = DEC) {}

void log(const char cs[], bool withCarriageReturn = false) {
  File dataFile = SD.open("console.log", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    if (withCarriageReturn) {
      dataFile.println(cs);
    } else {
      dataFile.print(cs);
    }
    dataFile.close();
  } else {
    Serial.println("Error while trying to log to SD card");
  }
}

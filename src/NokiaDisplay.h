#ifndef NokiaDisplay_h
#define NokiaDisplay_h

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Arduino.h>

#define pLIGHT 7

class NokiaDisplay {
public:
  Adafruit_PCD8544 d = Adafruit_PCD8544(6, 7, 8, 9, 10);
  NokiaDisplay(int8_t *rawDataPointer) {
    // Software SPI (slower updates, more flexible pin options):
    // pin 7 - Serial clock out (SCLK)
    // pin 6 - Serial data out (DIN)
    // pin 5 - Data/Command select (D/C)
    // pin 4 - LCD chip select (CS)
    // pin 3 - LCD reset (RST)
    rawData = rawDataPointer;
  };
  void init();
  void update();

private:
  int8_t *rawData;
  void displayInfoMode();
  String getData(uint8_t key);
  void printArrow(int16_t x, int16_t y);
  void displayMenuMode();
};
#endif

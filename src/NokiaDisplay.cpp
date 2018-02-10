

#include "NokiaDisplay.h"
#include "ProjectData.h"
#include <Fonts/FreeSansBold24pt7b.h>
#include <SPI.h>

void NokiaDisplay::init() {
  pinMode(pLIGHT, OUTPUT);
  digitalWrite(pLIGHT, HIGH);
  d.begin();
  d.clearDisplay();
  d.setContrast(60);
}
void NokiaDisplay::update() {
  int8_t t = rawData[DISPLAY_MODE];
  digitalWrite(pLIGHT, t == DISPLAY_OFF);
  // if (t == DISPLAY_MENU)
  //   displayMenuMode();
  // else
    displayInfoMode();
}

String NokiaDisplay::getData(uint8_t key) {
  int8_t v = rawData[key];
  v = constrain(v, -10, 100);
  if (v < -9)
    return "-#"; // display can't display 3 digits value. Show error
  if (v > 99) {
    return "##"; //DHT READ ERROR for example
  }
  String res = String(v);
  if (key == ROOM_TEMP)
    return res;
  if (key == GAS_TARGET_TEMP && v == -1)
    return "--";
  if (v < 0)
    return res;
  if (v < 10)
    return "0" + res;
  return res;
}

void NokiaDisplay::printArrow(int16_t x, int16_t y) {
  d.drawPixel(x, y, BLACK);
  d.drawPixel(x + 1, y, BLACK);
  d.drawPixel(x + 2, y, BLACK);
  d.drawPixel(x + 3, y, BLACK);
  d.drawPixel(x + 4, y, BLACK);
  d.drawPixel(x + 5, y, BLACK);
  d.drawPixel(x + 4, y - 1, BLACK);
  d.drawPixel(x + 4, y + 1, BLACK);
}

void NokiaDisplay::displayInfoMode() {
  // prepare to new cycle
  d.clearDisplay();
  d.setFont(NULL);
  d.setTextSize(1);
  d.setTextColor(BLACK);
  d.setCursor(0, 0);
  // input temperature
  d.print(getData(GAS_TEMP_INPUT));
  d.drawPixel(12, 0, BLACK);

  printArrow(14, 6);
  d.drawRect(21, 0, 5, 8, BLACK);
  printArrow(27, 1);

  // output temperature
  d.setCursor(36, 0);
  d.print(getData(GAS_TEMP_OUTPUT));
  d.drawPixel(48, 0, BLACK);

  // humidity
  d.setCursor(66, 0);
  d.setTextSize(1);
  d.setTextColor(BLACK);
  d.print(getData(ROOM_HUMIDITY));
  d.print('%');

  // celcius char
  d.setCursor(78, 12);
  d.write(247);

  // motor position - gas temperature
  d.setCursor(0, 33);
  d.setTextSize(2);
  d.print(getData(GAS_TARGET_TEMP));

  if (rawData[SELECTED_MENU] == SELECTED_MENU_AUTOMATIC) {
    // climat control indicator
    d.setCursor(0, 24);
    d.setTextSize(1);
    d.print("auto");

    // climat target
    d.fillRoundRect(0, 11, 23, 11, 2, BLACK);
    d.setCursor(6, 13);
    d.setTextColor(WHITE);
    d.print(getData(CLIM_TARGET_TEMP));
  }
  if (rawData[SELECTED_MENU] == SELECTED_MENU_CALIBRATION) {
    d.setCursor(0, 20);
    d.setTextSize(2);
    d.setTextColor(BLACK);
    d.print("+-");
  }

  // room temperature
  int tx = 27;
  if (rawData[ROOM_TEMP] < 10)
    tx = 46;
  if (rawData[ROOM_TEMP] < 0)
    tx = 34;
  d.setTextSize(1);
  d.setFont(&FreeSansBold24pt7b);
  d.setCursor(tx, 46);
  d.setTextColor(BLACK);
  d.print(getData(ROOM_TEMP));
  d.display();
}

// void NokiaDisplay::displayMenuMode() {
//   d.clearDisplay();
//   d.setFont(NULL);
//   d.setTextSize(1);
//   d.setCursor(0, 10);
//
//   String menus[3];
//   menus[0] = "automatic";
//   menus[1] = "manual";
//   menus[2] = "calibration";
//   for (byte i = 0; i < 3; i++) {
//     if (rawData[SELECTED_MENU] == i)
//       d.setTextColor(WHITE, BLACK);
//     else
//       d.setTextColor(BLACK);
//     d.println(menus[i]);
//   }
//   d.display();
// }

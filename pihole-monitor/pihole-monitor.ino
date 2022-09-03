#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "Adafruit_Arcada.h"
#include <Fonts/FreeSansOblique9pt7b.h>
typedef volatile uint32_t REG32;
#define pREG32 (REG32 *)
#define DEVICE_ID_HIGH (*(pREG32(0x10000060)))
#define DEVICE_ID_LOW (*(pREG32(0x10000064)))
#define MAC_ADDRESS_HIGH (*(pREG32(0x100000a8)))
#define MAC_ADDRESS_LOW (*(pREG32(0x100000a4)))

const int LEFT_BUTTON = 5;
const int RIGHT_BUTTON = 11;

int current_color = 0;
int current_screen = 0;

bool left_button_state = false;
bool right_button_state = false;
bool trigger_pause = false;

const int PIHOLE_COLORS[] = {0xFFFF, 0x001F, 0xF800, 0x07E0, 0x7FF, 0xF81F, 0xFFE0};

char *numBlocked = strdup("0");
char *numQueries = strdup("0");
char *numBlockedToday = strdup("0");
char *pctBlockedPercent = strdup("0.0%");
int domains_over_time[150];
int ads_over_time[150];

#define BLACK 0x0000

BLEDis bledis;
BLEUart bleuart;

Adafruit_Arcada arcada;

#include "Adafruit_APDS9960.h"
#include <ArduinoJson.h>
Adafruit_APDS9960 apds;

char recvText[1] = "";

void setup()
{
  Serial.begin(115200);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);

  Bluefruit.autoConnLed(true);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();
  bleuart.begin();
  apds.begin();
  apds.enableProximity(true);
  apds.enableGesture(true);
  startAdv();
  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->fillScreen(ARCADA_BLACK);
  arcada.display->setCursor(0, 10);
  arcada.display->setTextColor(PIHOLE_COLORS[current_color]);
  arcada.display->setTextWrap(true);
  arcada.display->setTextSize(1);
  arcada.display->setFont(&FreeSansOblique9pt7b);

  redraw();
}

void startAdv(void)
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  Bluefruit.Advertising.addService(bleuart);

  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void loop()
{
  if (!Bluefruit.connected())
  {
    printMAC();
  }
  if (digitalRead(LEFT_BUTTON) == HIGH)
  {
    left_button_state = false;
  }
  else
  {
    if (left_button_state)
    {
      Serial.println("L Button clicked");
      current_color++;
      if (current_color >= 7)
      {
        current_color = 0;
      }
      arcada.display->setTextColor(PIHOLE_COLORS[current_color]);
      redraw();
    }
    left_button_state = true;
  }

  if (digitalRead(RIGHT_BUTTON) == HIGH)
  {
    right_button_state = false;
  }
  else
  {
    if (right_button_state)
    {
      Serial.println("R Button clicked");
//       trigger_pause = true;
    }
    right_button_state = true;
  }

  while (bleuart.available())
  {
    uint8_t ch;
    char cha = bleuart.read();
    ch = (uint8_t)cha;
    Serial.write(ch);
    strncat(recvText, &cha, 1);
    if (cha == '\n')
    {
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, recvText);
      free(numBlocked);
      free(numQueries);
      free(numBlockedToday);
      free(pctBlockedPercent);
      numBlocked = strdup(doc["dbb"]);
      numQueries = strdup(doc["dqt"]);
      numBlockedToday = strdup(doc["abt"]);
      pctBlockedPercent = strdup(doc["apt"]);
      copyArray(doc["dot"], domains_over_time);
      copyArray(doc["aot"], ads_over_time);
      // domains_over_time = doc["dot"];
      // ads_over_time = doc["aot"];
      char pct = '%';
      strncat(pctBlockedPercent, &pct, 1);
      redraw();
      if(trigger_pause) {
        trigger_pause = false;
        Serial.println("Sending pause");
        bleuart.write("5");
      }
      recvText[0] = '\0';
    }
  }

  uint8_t gesture = apds.readGesture();
  // Directions are wrong due to sensor orientation
  if (gesture == APDS9960_DOWN)
  {
    Serial.println("DOWN"); // ACTUALLY RIGHT
    current_screen++;
    if (current_screen > 2)
    {
      current_screen = 2;
    }
    redraw();
  }
  if (gesture == APDS9960_UP)
  {
    Serial.println("UP"); // ACTUALLY LEFT
    current_screen--;
    if (current_screen < 0)
    {
      current_screen = 0;
    }
    redraw();
  }
  if (gesture == APDS9960_LEFT)
  {
    Serial.println("LEFT"); // ACTUALLY DOWN
  }
  if (gesture == APDS9960_RIGHT)
  {
    Serial.println("RIGHT"); // ACTUALLY UP
  }
}

void printMAC()
{
  uint32_t addr_high = ((MAC_ADDRESS_HIGH)&0x0000ffff) | 0x0000c000;
  uint32_t addr_low = MAC_ADDRESS_LOW;
  Serial.print((addr_high >> 8) & 0xFF, HEX);
  Serial.print(":");
  Serial.print((addr_high)&0xFF, HEX);
  Serial.print(":");
  Serial.print((addr_low >> 24) & 0xFF, HEX);
  Serial.print(":");
  Serial.print((addr_low >> 16) & 0xFF, HEX);
  Serial.print(":");
  Serial.print((addr_low >> 8) & 0xFF, HEX);
  Serial.print(":");
  Serial.print((addr_low)&0xFF, HEX);
  Serial.println("");
}

void redraw()
{
  switch (current_screen)
  {
  case 0:
  {
    arcada.display->fillScreen(ARCADA_BLACK);
    uint16_t w;
    String domainsBlocked = "Domains Blocked";
    String queriesToday = "Queries Today";
    String blockedToday = "Ads Blocked Today";
    String blockedPercent = "% Queries Blocked Today";
    arcada.display->setTextSize(1);
    arcada.display->getTextBounds(domainsBlocked, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 20);
    arcada.display->println(domainsBlocked);
    arcada.display->setTextSize(2);
    arcada.display->getTextBounds(numBlocked, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 50);
    arcada.display->println(numBlocked);
    arcada.display->setTextSize(1);
    arcada.display->getTextBounds(queriesToday, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 80);
    arcada.display->println(queriesToday);
    arcada.display->setTextSize(2);
    arcada.display->getTextBounds(numQueries, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 110);
    arcada.display->println(numQueries);
    arcada.display->setTextSize(1);
    arcada.display->getTextBounds(blockedToday, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 140);
    arcada.display->println(blockedToday);
    arcada.display->setTextSize(2);
    arcada.display->getTextBounds(numBlockedToday, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 170);
    arcada.display->println(numBlockedToday);
    arcada.display->setTextSize(1);
    arcada.display->getTextBounds(blockedPercent, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 200);
    arcada.display->println(blockedPercent);
    arcada.display->setTextSize(2);
    arcada.display->getTextBounds(pctBlockedPercent, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 230);
    arcada.display->println(pctBlockedPercent);
    break;
  }
  case 1:
  {
    arcada.display->fillScreen(ARCADA_BLACK);
    uint16_t w;
    String queriesOverTime = "Queries Over Time";
    arcada.display->setTextSize(1);
    arcada.display->getTextBounds(queriesOverTime, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 20);
    arcada.display->println(queriesOverTime);
    int n = sizeof(domains_over_time) / sizeof(int);
    Serial.println(n);
    float largest = -1.0;
    for (int i = 0; i < n; i++)
    {
      if (largest < domains_over_time[i])
      {
        largest = domains_over_time[i] * 1.0;
      }
    }
    Serial.println(largest);
    for (int i = 0; i < n; i++)
    {
      int len = 200 * (domains_over_time[i] / largest);
      arcada.display->drawFastVLine(i, 239 - len, len, PIHOLE_COLORS[current_color]);
    }
    arcada.display->setCursor(0, 40);
    arcada.display->println(int(largest));
    arcada.display->setCursor(0, 239);
    arcada.display->println("0");
    break;
  }
  case 2:
  {
    arcada.display->fillScreen(ARCADA_BLACK);
    uint16_t w;
    String adsOverTime = "Ads Blocked Over Time";
    arcada.display->setTextSize(1);
    arcada.display->getTextBounds(adsOverTime, 0, 0, NULL, NULL, &w, NULL);
    arcada.display->setCursor(120 - (w / 2), 20);
    arcada.display->println(adsOverTime);
    int n = sizeof(ads_over_time) / sizeof(int);
    Serial.println(n);
    float largest = -1.0;
    for (int i = 0; i < n; i++)
    {
      if (largest < ads_over_time[i])
      {
        largest = ads_over_time[i] * 1.0;
      }
    }
    Serial.println(largest);
    for (int i = 0; i < n; i++)
    {
      int len = 200 * (ads_over_time[i] / largest);
      arcada.display->drawFastVLine(i, 239 - len, len, PIHOLE_COLORS[current_color]);
    }
    arcada.display->setCursor(0, 40);
    arcada.display->println(int(largest));
    arcada.display->setCursor(0, 239);
    arcada.display->println("0");
    break;
    break;
  }
  }
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection *connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = {0};
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
   Callback invoked when a connection is dropped
   @param conn_handle connection where this event happens
   @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void)conn_handle;
  (void)reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
}

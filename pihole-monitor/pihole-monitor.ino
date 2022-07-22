/*********************************************************************
  This is an example for our nRF52 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "Adafruit_Arcada.h"
#include <Fonts/FreeSansOblique9pt7b.h>

// Color definitions

const int LEFT_BUTTON = 5;

int color = 0;

bool left_button_state = false;

const int COLORS[] ={0x001F,0xF800,0x07E0,0x7FF,0xF81F,0xFFE0,0xFFFF};

#define BLACK    0x0000

// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery


Adafruit_Arcada arcada;

#include "Adafruit_APDS9960.h"
Adafruit_APDS9960 apds;

char recvText[1] = "";



void setup()
{
  Serial.begin(115200);

#if CFG_DEBUG
  // Blocking wait for connection when debug mode is enabled via IDE
  while ( !Serial ) yield();
#endif

  Serial.println("Bluefruit52 BLEUART Example");
  Serial.println("---------------------------\n");

  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behavior, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  apds.begin();


  //gesture mode will be entered once proximity mode senses something close
  apds.enableProximity(true);
  apds.enableGesture(true);

  // Set up and start advertising
  startAdv();

  arcada.displayBegin();

  // Turn on backlight
  arcada.setBacklight(255);
  arcada.display->fillScreen(ARCADA_BLACK);
  arcada.display->setCursor(0, 10);
  arcada.display->setTextColor(COLORS[3]);
  arcada.display->setTextWrap(true);
  arcada.display->setTextSize(1);
  arcada.display->setFont(&FreeSansOblique9pt7b);

  Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
  Serial.println("Once connected, enter character(s) that you wish to send");
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
     - Enable auto advertising if disconnected
     - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     - Timeout for fast mode is 30 seconds
     - Start(timeout) with timeout = 0 will advertise forever (until connected)

     For recommended advertising interval
     https://developer.apple.com/library/content/qa/qa1931/_index.html
  */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void loop()
{
  // Forward data from HW Serial to BLEUART
  while (Serial.available())
  {
    // Delay to wait for enough input, since we have a limited transmission buffer
    delay(2);

    uint8_t buf[64];
    int count = Serial.readBytes(buf, sizeof(buf));
    bleuart.write( buf, count );
  }

  if(digitalRead(LEFT_BUTTON)== HIGH){
    // color++;
    // arcada.display->setTextColor(COLORS[color]);
  }

  // Forward from BLEUART to HW Serial
  while ( bleuart.available() )
  {
    uint8_t ch;
    char cha = bleuart.read();
    ch = (uint8_t) cha;
    Serial.write(ch);
    strncat(recvText, &cha, 1);
    if (cha == '\n') {
      String text = String(recvText);
      int index = text.indexOf(';');
      String numBlocked = text.substring(0, index);
      int newIndex = text.indexOf(';', index + 1);
      String numQueries = text.substring(index+1, newIndex);
      index = newIndex;
      newIndex = text.indexOf(';', index + 1);
      String numBlockedToday = text.substring(index+1, newIndex);
      index = newIndex;
      newIndex = text.indexOf('\n', index + 1);
      String pctBlockedPercent = text.substring(index+1, newIndex);
      pctBlockedPercent += '%';
      arcada.display->fillScreen(ARCADA_BLACK);
      uint16_t w;
      String domainsBlocked = "Domains Blocked";
      String queriesToday = "Queries Today";
      String blockedToday = "Ads Blocked Today";
      String blockedPercent = "% Ads Blocked Today";
      arcada.display->setTextSize(1);
      arcada.display->getTextBounds(domainsBlocked, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 20);
      arcada.display->println(domainsBlocked);
      arcada.display->setTextSize(2);
      arcada.display->getTextBounds(numBlocked, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 50);
      arcada.display->println(numBlocked);
      arcada.display->setTextSize(1);
      arcada.display->getTextBounds(queriesToday, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 80);
      arcada.display->println(queriesToday);
      arcada.display->setTextSize(2);
      arcada.display->getTextBounds(numQueries, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 110);
      arcada.display->println(numQueries);
      arcada.display->setTextSize(1);
      arcada.display->getTextBounds(blockedToday, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 140);
      arcada.display->println(blockedToday);
      arcada.display->setTextSize(2);
      arcada.display->getTextBounds(numBlockedToday, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 170);
      arcada.display->println(numBlockedToday);
      arcada.display->setTextSize(1);
      arcada.display->getTextBounds(blockedPercent, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 200);
      arcada.display->println(blockedPercent);
      arcada.display->setTextSize(2);
      arcada.display->getTextBounds(pctBlockedPercent, 0, 0, NULL, NULL, &w, NULL);
      arcada.display->setCursor(120-(w/2), 230);
      arcada.display->println(pctBlockedPercent);
      recvText[0]='\0';
    }

  }

  uint8_t gesture = apds.readGesture();
  if (gesture == APDS9960_DOWN) Serial.println("v");
  if (gesture == APDS9960_UP) Serial.println("^");
  if (gesture == APDS9960_LEFT) Serial.println("<");
  if (gesture == APDS9960_RIGHT) Serial.println(">");
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
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
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

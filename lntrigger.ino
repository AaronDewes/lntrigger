

/*               LOAD LIBRARIES               */

#include <M5Stack.h>
#include "FS.h"
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "SPIFFS.h"

/*               SOME DEFINES                 */

#define LED_PIN 25

/*               SOME VARIABLES               */

char lnbits_server[40] = "https://lnbits.com";
char lnbits_port[10] = "443";
char invoice_key[500] = "";
char lnbits_description[100] = "";
char lnbits_amount[500] = "1000";
char high_pin[5] = "16";
char time_pin[20] = "3000";
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";
String payReq = "";
String dataId = "";
bool paid = false;
bool shouldSaveConfig = false;
bool down = false;

const char *spiffcontent = "";
String spiffing;

/*                   SETUP                     */

void setup()
{
  M5.begin();
  logo_screen();
  delay(3000);
  lnbits_screen();
  // START PORTAL
  portal();
}

/*             HELPER FUNCTIONS                */

bool StartsWith(const char *a, const char *b)
{
  if (strncmp(a, b, strlen(b)) == 0)
    return 1;
  return 0;
}

void stop()
{
  while(1);
}

/*                 MAIN LOOP                   */

void loop()
{
  pinMode(atoi(high_pin), OUTPUT);
  digitalWrite(atoi(high_pin), LOW);
  if (String(invoice_key) == "")
  {
    error_no_invoice_key_screen();
  }
  Serial.println(String(invoice_key));
  getinvoice();

  if (down)
  {
    error_no_connection_screen();
    getinvoice();
    delay(5000);
  }
  if (payReq != "")
  {
    qrdisplay_screen();
    delay(5000);
  }
  while (paid == false && payReq != "")
  {
    checkinvoice();
    if (paid)
    {
      complete_screen();
      digitalWrite(atoi(high_pin), HIGH);
      delay(atoi(time_pin));
      digitalWrite(atoi(high_pin), LOW);
    }
    delay(3000);
  }
  payReq = "";
  dataId = "";
  paid = false;
  delay(4000);
}

/*               DISPLAY                 */

void logo_screen()
{
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_PURPLE);
  M5.Lcd.println("LNbitsTrigger");
}

void processing_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(40, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PROCESSING");
}

void lnbits_screen()
{
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.println("POWERED BY LNBITS");
}

void portal_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(30, 80);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PORTAL LAUNCHED");
}

void complete_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(60, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("COMPLETE");
}

void error_no_invoice_key_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(70, 80);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("NO INVOICE KEY");
}

void error_no_connection_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(70, 80);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("CAN'T CONNECT TO LNBITS");
}

void fs_error_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(70, 80);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("FILESYSTEM ERROR");
}

void qrdisplay_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.qrcode(payReq, 45, 0, 240, 14);
  delay(100);
}

/*               NODE CALLS                 */

void getinvoice()
{
  const char *lnbitsserver = lnbits_server;
  const char *lnbitsport = lnbits_port;
  const char *invoicekey = invoice_key;
  const char *lnbitsamount = lnbits_amount;
  const char *lnbitsdescription = lnbits_description;
  bool ssl = StartsWith(lnbitsserver, "https://");
  WiFiClient *client;
  client = (ssl) ? new WiFiClientSecure() : new WiFiClient();

  if (!client->connect(lnbitsserver, atoi(lnbitsport)))
  {
    down = true;
    return;
  }

  String topost = '{"out": false,"amount": ' + String(lnbitsamount) + ', "memo" : "' + String(lnbitsdescription) + String(random(1, 1000)) + '"}';
  String url = "/api/v1/payments";
  client->println(String("POST ") + url + " HTTP/1.1");
  client->println(String("Host: ") + lnbitsserver);
  client->println("User-Agent: LNTrigger");
  client->println(String("X-Api-Key: ") + invoicekey);
  client->println("Content-Type: application/json");
  client->println("Connection: close");
  client->println(String("Content-Length: ") + topost.length());
  client->println();
  client->println();
  client->println(topost);
  client->println();
  while (client->connected())
  {
    String line = client->readStringUntil('\n');
    if (line == "\r")
    {
      break;
    }
    if (line == "\r")
    {
      break;
    }
  }

  String line = client->readString();

  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char *payment_hash = doc["checking_id"];
  const char *payment_request = doc["payment_request"];
  payReq = payment_request;
  dataId = payment_hash;
}

void checkinvoice()
{
  const char *lnbitsserver = lnbits_server;
  const char *lnbitsport = lnbits_port;
  const char *invoicekey = invoice_key;
  bool ssl = StartsWith(lnbitsserver, "https://");
  WiFiClient *client;
  client = (ssl) ? new WiFiClientSecure() : new WiFiClient();

  if (!client->connect(lnbitsserver, atoi(lnbitsport)))
  {
    down = true;
    return;
  }

  String url = "/api/v1/payments/";
  client->println(String("GET ") + url + dataId + " HTTP/1.1");
  client->println(String("Host: ") + lnbitsserver);
  client->println("User-Agent: LNTrigger");
  client->println(String("X-Api-Key: ") + invoicekey);
  client->println("Connection: close");
  client->println();
  client->println();
  while (client->connected())
  {
    String line = client->readStringUntil('\n');
    if (line == "\r")
    {
      break;
    }
    if (line == "\r")
    {
      break;
    }
  }
  String line = client->readString();
  Serial.println(line);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  bool charPaid = doc["paid"];
  paid = charPaid;
}

bool InitalizeFileSystem() {
  bool initok = false;
  initok = SPIFFS.begin();
  if (!(initok))
  {
    Serial.println("Formatting SPI filesystem");
    SPIFFS.format();
    initok = SPIFFS.begin();
  }
  if (!(initok))
  {
    SPIFFS.format();
    initok = SPIFFS.begin();
  }
  if (initok) { Serial.println("Filesystem mounted"); } else { Serial.println("failed to mount FS"); }
  return initok;
}

void portal()
{

  WiFiManager wm;
  Serial.println("mounting FS...");
  if (!InitalizeFileSystem())
  {
    Serial.println("failed to mount FS");
    fs_error_screen();
    delay(2000);
    stop();
  }

  // Check if a reset was triggered, wipe data if it was
  for (int i = 0; i <= 100; i++)
  {
    if (M5.BtnA.wasPressed())
    {
      portal_screen();
      delay(1000);
      File file = SPIFFS.open("/config.json", FILE_WRITE);
      file.print("{}");
      wm.resetSettings();
      i = 100;
    }
    delay(50);
    M5.update();
  }

  // Mount the file system and read config.json
  File file = SPIFFS.open("/config.json");

  spiffing = file.readStringUntil('\n');
  spiffcontent = spiffing.c_str();
  DynamicJsonDocument json(1024);
  deserializeJson(json, spiffcontent);
  if (!json["lnbits_server"])
  {
    strcpy(lnbits_server, json["lnbits_server"]);
    strcpy(lnbits_port, json["lnbits_port"]);
    strcpy(lnbits_description, json["lnbits_description"]);
    strcpy(invoice_key, json["invoice_key"]);
    strcpy(lnbits_amount, json["lnbits_amount"]);
    strcpy(high_pin, json["high_pin"]);
    strcpy(time_pin, json["time_pin"]);
  }

  // ADD PARAMS TO WIFIMANAGER
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_lnbits_server("server", "LNbits server", lnbits_server, 40);
  WiFiManagerParameter custom_lnbits_port("port", "LNbits port", lnbits_port, 10);
  WiFiManagerParameter custom_lnbits_description("description", "Memo", lnbits_description, 200);
  WiFiManagerParameter custom_invoice_key("invoice", "LNbits invoice key", invoice_key, 500);
  WiFiManagerParameter custom_lnbits_amount("amount", "Amount to charge (sats)", lnbits_amount, 10);
  WiFiManagerParameter custom_high_pin("high", "Pin to turn on", high_pin, 5);
  WiFiManagerParameter custom_time_pin("time", "Time for pin to turn on for (millisecs)", time_pin, 20);
  wm.addParameter(&custom_lnbits_server);
  wm.addParameter(&custom_lnbits_port);
  wm.addParameter(&custom_lnbits_description);
  wm.addParameter(&custom_invoice_key);
  wm.addParameter(&custom_lnbits_amount);
  wm.addParameter(&custom_high_pin);
  wm.addParameter(&custom_time_pin);

  // If a reset was triggered, run the portal and write files
  if (!wm.autoConnect("⚡lntrigger⚡", "password1"))
  {
    Serial.println("Failed to connect and hit timeout.");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("Connected successfully.");
  strcpy(lnbits_server, custom_lnbits_server.getValue());
  strcpy(lnbits_port, custom_lnbits_port.getValue());
  strcpy(lnbits_description, custom_lnbits_description.getValue());
  strcpy(invoice_key, custom_invoice_key.getValue());
  strcpy(lnbits_amount, custom_lnbits_amount.getValue());
  strcpy(high_pin, custom_high_pin.getValue());
  strcpy(time_pin, custom_time_pin.getValue());
  if (shouldSaveConfig)
  {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["lnbits_server"] = lnbits_server;
    json["lnbits_port"] = lnbits_port;
    json["lnbits_description"] = lnbits_description;
    json["invoice_key"] = invoice_key;
    json["lnbits_amount"] = lnbits_amount;
    json["high_pin"] = high_pin;
    json["time_pin"] = time_pin;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("Failed to open config file for writing.");
    }
    serializeJsonPretty(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    shouldSaveConfig = false;
  }

  Serial.println("Local IP");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}

void saveConfigCallback()
{
  processing_screen();
  Serial.println("Should save config.");
  shouldSaveConfig = true;
}

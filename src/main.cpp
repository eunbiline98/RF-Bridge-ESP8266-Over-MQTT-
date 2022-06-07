#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>

const char *SSID = "xxxxxxx";
const char *PASS = "xxxxxxxxx";

const char *MQTT_SERVER = "xxxxxxxx";
const char *MQTT_STATUS = "tele/esp8266RF/LWT";

const char *SWITCH_1_CMD = "esp8266RF/cmnd/switch1";
const char *SWITCH_2_CMD = "esp8266RF/cmnd/switch2";
const char *DECODE_RF_CMD = "esp8266RF/cmnd/decode";

String MQTT_CLIENT_ID = "esp8266RF-";
const char *MQTT_USERNAME = "xxxxxx";
const char *MQTT_PASSWORD = "xxxxxxxx";

static const char *bin2tristate(const char *bin);
static char *dec2binWzerofill(unsigned long Dec, unsigned int bitLength);

String decodeVal;

SSD1306Wire display(0x3c, SDA, SCL);
RCSwitch mySwitch = RCSwitch();
WiFiClient espClient;
PubSubClient client(espClient);

void output(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int *raw, unsigned int protocol)
{

  const char *bin = dec2binWzerofill(decimal, length);
  Serial.print("Decimal: ");
  Serial.print(decimal);
  Serial.print(" (Bit)");
  Serial.print(length);
  Serial.println();
  Serial.print("Binary: ");
  Serial.print(bin);
  Serial.println();
  Serial.print("Tri-State: ");
  Serial.print(bin2tristate(bin));
  Serial.println();
  Serial.print("PulseLength: ");
  Serial.print(delay);
  Serial.print(" m/s");
  Serial.println();
  Serial.print("Protocol: ");
  Serial.println(protocol);

  // display.flipScreenVertically();
  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.setFont(ArialMT_Plain_24);
  // display.drawString(0, 0, "Binary : ");
  // display.setFont(ArialMT_Plain_24);
  // display.drawString(0, 24, " ");
  // display.setFont(ArialMT_Plain_10);
  // display.drawString(0, 48, bin);
  // display.display();

  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0, "Status : ");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 30, "Push MQTT");
  display.display();
  display.clear();

  client.publish(DECODE_RF_CMD, String(bin).c_str());

  // Serial.print("Raw data: ");
  // for (unsigned int i = 0; i <= length * 2; i++)
  // {
  //   Serial.print(raw[i]);
  //   Serial.print(",");
  // }
  Serial.println();
  Serial.println();
}

static const char *bin2tristate(const char *bin)
{
  static char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos] != '\0' && bin[pos + 1] != '\0')
  {
    if (bin[pos] == '0' && bin[pos + 1] == '0')
    {
      returnValue[pos2] = '0';
    }
    else if (bin[pos] == '1' && bin[pos + 1] == '1')
    {
      returnValue[pos2] = '1';
    }
    else if (bin[pos] == '0' && bin[pos + 1] == '1')
    {
      returnValue[pos2] = 'F';
    }
    else
    {
      return "not applicable";
    }
    pos = pos + 2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
}

static char *dec2binWzerofill(unsigned long Dec, unsigned int bitLength)
{
  static char bin[64];
  unsigned int i = 0;

  while (Dec > 0)
  {
    bin[32 + i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j < bitLength; j++)
  {
    if (j >= bitLength - i)
    {
      bin[j] = bin[31 + i - (j - (bitLength - i))];
    }
    else
    {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';

  return bin;
}

void decode()
{

  if (mySwitch.available())
  {
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(), mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
  }
  else
  {
    digitalWrite(2, LOW);
    display.clear();
    display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "Status : ");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Ready...");
    display.display();
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    MQTT_CLIENT_ID += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID.c_str(), MQTT_USERNAME, MQTT_PASSWORD, MQTT_STATUS, 1, 1, "Offline"))
    {
      Serial.println("connected");
      // subscribe
      client.subscribe(SWITCH_1_CMD);
      client.subscribe(SWITCH_2_CMD);
      client.subscribe(DECODE_RF_CMD);
      client.publish(MQTT_STATUS, "Online", true);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 0, "Status : ");
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 26, "Disconnect");
      display.display();
      delay(5000);
    }
  }
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String payload;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    Serial.println();
    if ((char)message[i] != '"')
      payload += (char)message[i];
  }

  if (String(topic) == SWITCH_1_CMD)
  {
    if (payload == "1")
    {
      mySwitch.send(String(decodeVal).c_str());
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 0, "Status : ");
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 30, "Get MQTT");
      display.display();
      delay(2000);
      display.clear();
    }
    else if (payload == "0")
    {
      mySwitch.send(String(decodeVal).c_str());
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 0, "Status : ");
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 30, "Get MQTT");
      display.display();
      delay(2000);
      display.clear();
    }
  }

  if (String(topic) == DECODE_RF_CMD)
  {
    decodeVal = payload;
  }
}

void setup()
{
  Serial.begin(115200);
  display.init();
  // wait for 30 seconds, to prepare AP ready use
  for (int i = 5; i >= 0; i--)
  {
    display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "Waiting..");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 30, String(i).c_str());
    display.display();
    delay(1000);
    display.clear();
  }
  display.clear();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  mySwitch.enableReceive(0);   // Receiver on interrupt 0 => D3
  mySwitch.enableTransmit(14); // Receiver on interrupt 14 => D5
  pinMode(2, OUTPUT);

  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0, "  RF Bridge");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 30, "  Philoin V1");
  display.display();
  delay(5000);
  display.clear();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  client.setCallback(callback);
  client.setServer(MQTT_SERVER, 1883);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address:  ");
  Serial.println(WiFi.macAddress());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0, "Connected..");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 30, WiFi.localIP().toString());
  display.display();
  delay(3000);
  display.clear();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  decode();
  Serial.print("Decode Val :");
  Serial.println(decodeVal);
}
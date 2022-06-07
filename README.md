This guide shows how to use an ESP8266 to decode 433 MHz signals from RF remotes, and send them with an Arduino and a 433 MHz transmitter to remotely control all RF Device 433 MHz.

# RF-Bridge-ESP8266-Over-MQTT-
RF Bridge ESP8266 Decode Binary and Publish Decode With MQTT

# Hardware
- Esp8266 (Wemos D1 Mini) / Chip SoC Esp8266
- RF Module 433 MHz / 315 MHz (
adjust to the specs of your remote that will be decoded) 
- Oled Display (Optional)

# Software
- VsCode
- Platfromio
- MQTTBox
- Node-red (MQTT Broker Using Aedes Or Something Else)

# pinout
- Receiver 
- VCC -> 5V
- GND -> GND
- Data -> GPIO 0 // D3

Trnasmiter 
VCC -> 5V
GND -> GND
Data -> GPIO 14 // D5

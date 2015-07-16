#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LPD6803.h>
#include <Ticker.h>

//wifi access point
const char* ssid = "area3001";
const char* password = "**";

// Number of RGB LEDs in strand:
int nLEDs = 64;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 13;
int clockPin = 14;

// ESP stuff
MDNSResponder mdns;
ESP8266WebServer server(80);

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD6803 strip = LPD6803(nLEDs, dataPin, clockPin);

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound(){
  digitalWrite(statusLed, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(statusLed, 0);
}

void setColor(){
  if((server.args() != 3) | server.argName(0) != "R" | server.argName(1) != "G" | server.argName(2) != "B")
  {
    server.send(404, "text/plain", "er moeten 3 argumenten zijn: R, G en B");
    return;
  }
  else
  {
    int red = server.arg(0).toInt();
    int green = server.arg(1).toInt();
    int blue = server.arg(2).toInt();

    Serial.print("settting color to (RGB): ");
    Serial.print(red, DEC);
    Serial.print(",");
    Serial.print(green, DEC);
    Serial.print(",");
    Serial.println(blue, DEC);
    colorSet(Color(red, green, blue));
  }
}
 
void setup(void){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  server.on("/setColor", setColor);
  server.onNotFound(handleNotFound);
  
  //start server
  server.begin();
  Serial.println("HTTP server started");

  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  Serial.println("LEDS initalized");
}
 
void loop(void){
  server.handleClient();
} 

// fill the dots with said color
// good for testing purposes
void colorSet(uint16_t c) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }
  strip.show();
}

/* Helper functions */

// Create a 15 bit color value from R,G,B
unsigned int Color(byte r, byte g, byte b)
{
  //Take the lowest 5 bits of each value and append them end to end
  //return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F);
  unsigned int data;
  
  data = g & 0x1F;
  data <<= 5;
  data |= b & 0x1F;
  data <<= 5;
  data |= r & 0x1F;
  data |= 0x8000;

  return data;
}
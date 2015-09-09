#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LPD6803.h>
#include <Ticker.h>
#include <PubSubClient.h>

#include "ets_sys.h"
#include "osapi.h"

#define USE_US_TIMER

//wifi access point
const char* ssid = "area3001";
const char* password = "hackerspace";

// Number of RGB LEDs in strand:
int nLEDs = 64;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 13;
int clockPin = 14;

// ESP stuff
MDNSResponder mdns;
ESP8266WebServer server(80);

//IPAddress mqttserver(192, 168, 1, 146);
IPAddress mqttserver(128, 199, 61, 79);
WiFiClient mqttwclient;
PubSubClient mqttclient(mqttwclient, mqttserver);

#define BUFFER_SIZE 100
#define MQTT_TOPIC_SET_PIXEL_COLOR "leds/setPixelColor"
#define MQTT_TOPIC_SET_COLOR "leds/setColor"
#define MQTT_TOPIC_SET_COLORS "leds/setColors"

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD6803 strip = LPD6803(nLEDs, dataPin, clockPin);

uint32_t hex2int (const char* i_p_string, uint16_t i_nr_digits)
{
  uint32_t value = 0;
  for (uint16_t i=0; i<i_nr_digits; ++i)
  {
    uint8_t digit = 0;
    if (48 < i_p_string [i]&& 58 > i_p_string [i])
    {
      digit = i_p_string [i] - 48;
    }
    else if (64 < i_p_string [i] && 71 > i_p_string [i])
    {
      digit = i_p_string [i] - 55;
    }
    else if (96 < i_p_string [i] && 103 > i_p_string [i])
    {
      digit = i_p_string [i] - 87;
    }
    value = 16*value + digit;
  }

  return value;
}

uint16_t index2led (uint16_t i_index)
{
  uint16_t row = i_index / 8;
  uint16_t col = i_index % 8;
  uint16_t odd_row = row % 2;
  
  return row*8 + odd_row*col + (1-odd_row)*(7-col);
}


void mqttCallback (const MQTT::Publish& pub)
{
  uint8_t buf[BUFFER_SIZE];
  
  Serial.print(pub.topic());
  Serial.print(" => ");
  if (pub.has_stream())
  {
    
    int read;
    while (read = pub.payload_stream()->read(buf, BUFFER_SIZE)) {
      Serial.write(buf, read);
    }
    pub.payload_stream()->stop();
    Serial.println("");
  } else
    Serial.println(pub.payload_string());

  if (0 == strcmp (pub.topic ().c_str (), MQTT_TOPIC_SET_COLOR))
  {
    char in_buffer [12];
    pub.payload_string ().toCharArray (in_buffer, 12);
    in_buffer [3] = 0;
    in_buffer [7] = 0;
    in_buffer [11] = 0;

    LPD6803::color_t color;
    color.r = atoi (&in_buffer [0]) >> 3;
    color.g = atoi (&in_buffer [4]) >> 3;
    color.b = atoi (&in_buffer [8]) >> 3;
    
    Serial.print (color.r, DEC);
    Serial.print (", ");
    Serial.print (color.g, DEC);
    Serial.print (", ");
    Serial.println (color.b, DEC);

    for (int i=0; i < strip.getNrPixels (); ++i)
    {
        strip.setPixelColor (i, color);
    }
    strip.show ();
  }
  else if (0 == strcmp (pub.topic ().c_str (), MQTT_TOPIC_SET_PIXEL_COLOR))
  {
    Serial.print ("MQTT_TOPIC_SET_PIXEL_COLOR ... ");
    
    const char* p_digits = pub.payload_string ().c_str ();
    LPD6803::color_t color;

    uint16_t index = static_cast <uint16_t> (hex2int (&(p_digits [0]), 2));
    uint32_t color_value = hex2int (&(p_digits [2]), 6);
    color.r = (color_value & 0xFF0000) >> 19;
    color.g = (color_value & 0x00FF00) >> 11;
    color.b = (color_value & 0x0000FF) >> 3;

    strip.setPixelColor (index2led (index), color);

    strip.show ();  

    Serial.println ("done");
  }
  else if (0 == strcmp (pub.topic ().c_str (), MQTT_TOPIC_SET_COLORS))
  {

  }
}

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound(){
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
}

void setColor()
{
  Serial.print ("setColor ... ");
  
  LPD6803::color_t color;
  color.r = 0;
  color.g = 0;
  color.b = 0;

  if (server.hasArg ("R"))
  {
    color.r = server.arg ("R").toInt () >> 3;
  }
  if (server.hasArg ("G"))
  {
    color.g = server.arg ("G").toInt () >> 3;
  }
  if (server.hasArg ("B"))
  {
    color.b = server.arg ("B").toInt () >> 3;
  }

  server.send(200, "text/plain", "");

  for (int i=0; i < strip.getNrPixels (); ++i)
  {
      strip.setPixelColor (i, color);
  }
  strip.show ();

  Serial.println ("done");
}

void setPixelColor()
{
  Serial.print ("setPixelColor ... ");
  
  int index = 0;
  LPD6803::color_t color;
  color.r = 0;
  color.g = 0;
  color.b = 0;

  bool accept_call = true;
  if (server.hasArg ("i"))
  {
     index = server.arg ("i").toInt ();

     int row = index / 8;
     int col = index % 8;
     int odd_row = row % 2;

     index = row*8 + odd_row*col + (1-odd_row)*(7-col);
  }
  else
  {
    accept_call = false;
  }

  if (server.hasArg ("R"))
  {
    color.r = server.arg ("R").toInt () >> 3;
  }
  if (server.hasArg ("G"))
  {
    color.g = server.arg ("G").toInt () >> 3;
  }
  if (server.hasArg ("B"))
  {
    color.b = server.arg ("B").toInt () >> 3;
  }

  if (accept_call)
  {
    server.send(200, "text/plain", "");
  }
  else
  {
    server.send(404, "text/plain", "i (0-based index) and R, G and B (dec, optional)");
  }

  strip.setPixelColor (index, color);
  strip.show (index, index + 1);

  Serial.println ("done");
}

void setColors ()
{
  Serial.print ("setColors ... ");

  if (!server.hasArg ("c"))
  {
     server.send(404, "text/plain", "c= and then some hex color values");
  }

  const char* p_digits = server.arg ("c").c_str ();
  uint16_t nr_digits = server.arg ("c").length ();
  //int led_index;
  LPD6803::color_t color;
  
  for (uint16_t i = 0, index = 0; (i < nr_digits) && (index < 64); i += 6, ++index)
  {    
    uint32_t color_value = hex2int (&(p_digits [i]), 6);
    color.r = (color_value & 0xFF0000) >> 19;
    color.g = (color_value & 0x00FF00) >> 11;
    color.b = (color_value & 0x0000FF) >> 3;

    strip.setPixelColor (index2led (index), color);
  }

  server.send(200, "text/plain", "");

  strip.show ();

  Serial.println ("done");
}

 
void setup ()
{  
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

  if (!mqttclient.connected())
  {
      if (mqttclient.connect("ESP8266Client"))
      {
        mqttclient.set_callback(mqttCallback);
        mqttclient.subscribe (MQTT_TOPIC_SET_COLOR);
        mqttclient.subscribe (MQTT_TOPIC_SET_PIXEL_COLOR);
        mqttclient.subscribe (MQTT_TOPIC_SET_COLORS);

        Serial.println ("MQTT connected");
      }
      else
      {
        Serial.println ("MQTT connect FAIL");
      }
    }


   // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  Serial.println("LEDS initalized");

  timer1_attachInterrupt (OnTimerOneInterrupt);
  timer1_write (800);
  timer1_enable (TIM_DIV1, TIM_EDGE, TIM_LOOP);

  // configure server
  server.on("/", handleRoot);
  server.on("/setColor", setColor);
  server.on("/setColors", setColors);
  server.on("/setPixelColor", setPixelColor);
  server.onNotFound(handleNotFound);
  
  //start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{
  if (strip.outputReady ())
  {
    server.handleClient();
  }

  
  if (mqttclient.connected())
  {
    mqttclient.loop();
  }
  
  delay (0);
}

void OnTimerOneInterrupt ()
{
  strip.update ();
}

// fill the dots with said color
// good for testing purposes
void colorSet(LPD6803::color_t i_color)
{
  
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

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
const char* password = "**";

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
#define MQTT_TOPIC_SET_PIXEL_COLORS "leds/setPixelColors"
#define MQTT_TOPIC_SET_COLOR "leds/setColor"

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD6803 strip = LPD6803(nLEDs, dataPin, clockPin);

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
    
    /*uint8_t values [3];
    char* p_token;
    pub.payload_string ().toCharArray (reinterpret_cast <char*> (buf), pub.payload_string ().length () + 1);
    buf [pub.payload_string ().length ()] = 0;
    p_token = strtok (reinterpret_cast <char*> (buf), " ");
    values [0] = atoi (p_token);
    for (size_t i=1; i<3; ++i)
    {
      p_token = strtok (0x0, " ");
      Serial.print (p_token);
      if (0x0 == p_token)
      {
        break;
      }
      values [i] = atoi (p_token);
    }

    for (size_t i=0; i<3; ++i)
    {
      Serial.print (values [i], DEC);
      Serial.print (", ");
    }
    Serial.println ("");*/
  }
  else if (0 == strcmp (pub.topic ().c_str (), MQTT_TOPIC_SET_PIXEL_COLORS))
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

  if (!mqttclient.connected()) {
      if (mqttclient.connect("ESP8266Client")) {
        mqttclient.set_callback(mqttCallback);
        //mqttclient.subscribe("leds/setPixelColors");
        mqttclient.subscribe("leds/setColor");

        Serial.println ("MQTT connected");
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

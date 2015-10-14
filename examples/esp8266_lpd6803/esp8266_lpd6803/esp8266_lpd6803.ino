#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LPD6803.h>
#include <PubSubClient.h>


#define USE_US_TIMER // is this still needed?

// ldp6803
#define LED_COUNT 64
#define LED_NR_COLS 8
#define PIN_DATA  13
#define PIN_CLOCK 14

// wifi
#define WIFI_SSID "area3001"
#define WIFI_PWD  "hackerspace"

// mqtt
#define MQTT_BUFFER_SIZE      124
#define MQTT_TOPIC_SET_COLORS "leds/setColors"

// modules
MDNSResponder mdns; // do we actually need this?
ESP8266WebServer http_server(80);
IPAddress    mqtt_server(128, 199, 61, 79);
WiFiClient   mqtt_wclient;
PubSubClient mqtt_client(mqtt_wclient, mqtt_server);
LPD6803      led_strip = LPD6803(LED_COUNT, PIN_DATA, PIN_CLOCK);

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
    else if (LED_COUNT < i_p_string [i] && 71 > i_p_string [i])
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
  uint16_t row = i_index / LED_NR_COLS;
  uint16_t col = i_index % LED_NR_COLS;
  uint16_t odd_row = row % 2;
  
  return row*8 + odd_row*col + (1-odd_row)*(7-col);
}

void decodeColorString (const uint16_t& i_index_begin, const uint16_t& i_repeat_count, const char* i_p_digits, const size_t& i_nr_digits)
{
  uint16_t nr_colors = i_nr_digits / 6;
  LPD6803::color_t color;
  color.r = 0;
  color.g = 0;
  color.b = 0;

  for (uint16_t i = 0, current_index = i_index_begin;
       i < i_nr_digits;
       i += 6, ++current_index)
  {
    uint32_t color_value = hex2int (&(i_p_digits [i]), 6);
    color.r = (color_value & 0xFF0000) >> 19;
    color.g = (color_value & 0x00FF00) >> 11;
    color.b = (color_value & 0x0000FF) >> 3;

    for (uint16_t r = 0, index = current_index;
         (r < i_repeat_count) && (index < LED_COUNT);
         ++r, index += nr_colors)
    {
      led_strip.setPixelColor (index2led (index), color);
    }
  }

  led_strip.show ();
}

void mqttCallback (const MQTT::Publish& i_pub)
{
  if (0 == strcmp (i_pub.topic ().c_str (), MQTT_TOPIC_SET_COLORS))
  {
    Serial.print ("MQTT_TOPIC_SET_COLORS ... ");

    if (4 > i_pub.payload_string ().length ())
    {
      const char* p_digits =  i_pub.payload_string ().c_str ();
      uint16_t index = static_cast <uint16_t> (hex2int (&(p_digits [0]), 2));
      uint16_t repeat = static_cast <uint16_t> (hex2int (&(p_digits [2]), 2));

      p_digits = &(p_digits [4]);
      uint16_t nr_digits = i_pub.payload_string ().length () - 4;

      decodeColorString (index, repeat, p_digits, nr_digits);

      Serial.println ("done");
    }
    else
    {
      Serial.println ("error");
    }   
  }
}

void httpHandleRoot ()
{
  String message = "Hello there cutie\n\n"
                   "Call me like this: setColors\n"
                   "And tell me:\n"
                   "i=display start index\n"
                   "r=nr of repeats\n"
                   "c=array of rgb colors in hex (min length is 6 digits)\n";
  http_server.send (200, "text/plain", message);
}

void httpHandleNotFound ()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += http_server.uri();
  message += "\nMethod: ";
  message += (http_server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += http_server.args();
  message += "\n";
  for (uint8_t i=0; i<http_server.args(); i++)
  {
    message += " " + http_server.argName(i) + ": " + http_server.arg(i) + "\n";
  }
  http_server.send(404, "text/plain", message);
}

void httpHandleSetColors ()
{
  Serial.print ("setColors ... ");

  uint16_t index       = 0;
  uint16_t repeat      = 0;
  uint16_t nr_digits   = 0;
  uint16_t nr_colors   = 0;
  const char* p_digits = 0x0;
  if (http_server.hasArg ("i"))
  {
    index = http_server.arg ("i").toInt ();
  }
  if (http_server.hasArg ("r"))
  {
    repeat = http_server.arg ("r").toInt ();
  }
  if (http_server.hasArg ("c"))
  {
    p_digits = http_server.arg ("c").c_str ();
    nr_digits = http_server.arg ("c").length ();
    nr_colors = nr_digits / 6;
  }
  
  decodeColorString (index, repeat, p_digits, nr_digits);

  http_server.send (200, "text/plain", "");

  Serial.println ("done");
}

void httpHandleGetConfig ()
{
  Serial.print ("setColors ... ");

  String message = "{\"led_count\":";
  message += LED_COUNT;
  message += ",\"nr_cols\":";
  message += LED_NR_COLS;
  message += ",\"nr_rows\":";
  message += (LED_COUNT / LED_NR_COLS);
  message += "}";
  http_server.send (200, "application/json", message);
  
  Serial.println ("done");
}
 
void setup ()
{  
  // SERIAL
  Serial.begin (115200);
  Serial.println ("");

  // WIFI
  WiFi.begin (WIFI_SSID, WIFI_PWD);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay (500);
    Serial.print (".");
  }
  Serial.print ("\nConnected to ");
  Serial.println (WIFI_SSID);
  Serial.print ("IP address: ");
  Serial.println (WiFi.localIP());
  
  if (mdns.begin ("esp8266", WiFi.localIP()))
  {
    Serial.println ("MDNS responder started");
  }

   // LED STRIP
  led_strip.begin();
  led_strip.show();
  Serial.println("LEDS initalized");

  // MQTT CLIENT
  if (!mqtt_client.connected ())
  {
    if (mqtt_client.connect ("ESP8266Client"))
    {
      mqtt_client.set_callback(mqttCallback);
      mqtt_client.subscribe (MQTT_TOPIC_SET_COLORS);

      Serial.println ("MQTT connected");
    }
    else
    {
      Serial.println ("MQTT connect FAIL");
    }
  }

  // HTTP SERVER
  http_server.on ("/", httpHandleRoot);
  http_server.on ("/setColors", httpHandleSetColors);
  http_server.on ("/getConfig", httpHandleGetConfig);
  http_server.onNotFound (httpHandleNotFound);
  http_server.begin ();
  Serial.println ("HTTP server started");
}

void loop ()
{
  if (led_strip.outputReady ())
  {
    http_server.handleClient();
    if (mqtt_client.connected())
    {
      mqtt_client.loop();
    }
  }
    
  delay (0);
}


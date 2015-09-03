### LPD6803 Library for ESP8266

library for LP6803 ledstrips for ESP8266
fork of https://github.com/adafruit/LPD6803-RGB-Pixels

#### Connecting the LEDs <a name="connection"></a>

Make sure the I/O levels of you ESP8266 are 5V by using a driver or level translator. In this example we used the [TXB0102](http://www.ti.com/product/txb0102) voltage-level translator. Since the signal is being bit banged you can use any available GPIO pin. In our examples we use GPIO 13 as data pin and GPIO 14 as clock pin.

![Example Schematic](https://raw.githubusercontent.com/area3001/esp8266-arduino-lpd6803/master/img/connection.png)

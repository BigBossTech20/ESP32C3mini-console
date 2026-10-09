

# ESP32C3mini-console

**(THIS PROJECT IS STILL UNDER DEVELOPMENT!!)**

Simple dual-game handheld console based on **ESP32-C3 Mini** + **0.96" OLED**.



Features:

-DOOM

-TETRIS

-PING PONG

-Dino game

-Menu to select games

-Console saves your scores

-Code Is Fully Open-Source





---

### Pinout

| Component  | Pin  | Name | Function                          |
|------------|------|------|-----------------------------------|
| OK Button  | GP1  | OK   | Confirm / Jump / Shoot            |
| UP Button  | GP2  | Up   | Jump (Dino) / Move forward        |
| L Buttton  | GP3  | Left | Select in menu / Rotate left      |
| R Button   | GP5  | Right| Select in menu / Rotate right     |
| OLED       | GP6  | SDA  | Data cable to the screen          |
| OLED       | GP7  | SCL  | Clock cable to the screen         |
| OLED       | 3.3V | VCC  | Power for the screen              |
| OLED       | GND  | GND  | Ground                            |
---




### What You Need

- 4× Tactile buttons  
- 1× 0.96" I2C OLED  
- 1× ESP32-C3 Mini  
- Breadboard or PCB / perfboard  

---





Required Libaries:

-Wire.h

-Adafruit_GFX.h

-Adafruit_SSD1306.h

-Preferences.h

-math.h



Paste The libaries below:

https://github.com/adafruit/Adafruit-GFX-Library/blob/master/Adafruit_GFX.h

https://github.com/adafruit/Adafruit_BusIO

https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.h

https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/Wire/src/Wire.h

https://github.com/codebendercc/arduino-core-files/blob/master/v105/hardware/tools/avr/lib/avr/include/math.h

https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h

To  FILE --->  Preferences --->  Additional Boards Manager URLs 





### How to use Flash

1. Wire everything according to the pinout table above  

2.Download Any version from "releases" page

3. Flash the firmware  
  

Long press **OK** to return to the menu at any time.




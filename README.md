

# ESP32C3mini-console

**(THIS PROJECT IS STILL UNDER DEVELOPMENT!!)**

Simple dual-game handheld console based on **ESP32-C3 Mini** + **0.96" OLED**.

---

### Pinout

| Component  | Pin  | Name | Function                          |
|------------|------|------|-----------------------------------|
| ESP32-C3   | GP1  | OK   | Confirm / Jump / Shoot            |
| ESP32-C3   | GP2  | Up   | Jump (Dino) / Move forward        |
| ESP32-C3   | GP3  | Left | Select in menu / Rotate left      |
| ESP32-C3   | GP5  | Right| Select in menu / Rotate right     |
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

### How to use

1. Wire everything according to the pinout table above  
2. Flash the firmware  
3. Enjoy Dino + DOOM  

Long press **OK** to return to the menu at any time.

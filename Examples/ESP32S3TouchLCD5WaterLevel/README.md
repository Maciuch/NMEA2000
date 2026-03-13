# ESP32-S3 Touch LCD 5 Water Level Display

This example demonstrates how to read fluid level messages from the NMEA 2000 bus, specifically filtering for fresh water tanks, and rendering a live progress bar and percentage directly on the Waveshare ESP32-S3-Touch-LCD-5 display using LVGL.

It corresponds to the `M5AtomVL53L1XWaterLevel` example which acts as a sensor node broadcasting the water level. This sketch acts as the display node.

## Prerequisites
- [ESP32_Display_Panel](https://github.com/esp-arduino-libs/ESP32_Display_Panel) library installed via Arduino Library Manager
- [lvgl](https://github.com/lvgl/lvgl) library installed (v8.x recommended)
- Depending on your specific setup and LCD drivers from Waveshare, ensure you have correctly configured the `lv_port_disp_init` or display panel initialization functions.

## Setup
1. Upload `M5AtomVL53L1XWaterLevel.ino` to your M5 Atom with the VL53L1X sensor attached.
2. Upload this sketch to your `ESP32-S3-Touch-LCD-5` display board.
3. Connect both devices to the same NMEA 2000 bus.
4. The display will show the incoming fresh water level data in real-time.

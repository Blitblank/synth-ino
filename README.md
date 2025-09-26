This is firmware for the soburg_v2 board, a basic digital synthesizer based around the ESP32-S3 WROOM1 SoC.

Board characteristics: 
  - ESP32-s3 Wroom1 N4R8 Module
  - SSD1306 OLED Screen for waveform visualization
  - UART MIDI interface
  - PCM5102A 32 bit audio DAC utilizing I2S
  - SPI MicroSD storage for storing synthesizer voice profiles and wifi network info
  - MCP23017 I2C I/O expander for extra control potentiometers and status LEDs
  - Native USB
  - WS2812b LEDs and interface because I got bored 

Firmware stack: FreeRTOS >> ESP-IDF >> Arduino
Utilizing PlatformIO for build system and chip interfacing 



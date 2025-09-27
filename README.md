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

The ESP32 offers WiFi capabilities, which this project uses for a control interface instead of hardware circuitry.
The dual cores of the S3 module allows the synthesizer task to be isolated so that it is not blocked by other tasks (web server, scope, etc.)

Firmware stack: FreeRTOS >> ESP-IDF >> Arduino

Utilizing PlatformIO for build system and chip interfacing 

Future plans for soburg_v3: 
  - USB MIDI interface (through UART conversion)
  - ADC -> I2S conversion input for live sound modification effects

This synthesizer isn't very good, but it's neat :3 

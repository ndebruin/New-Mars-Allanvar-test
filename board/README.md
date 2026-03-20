# MARS Board Definitions

STM32H753ZIT6 based boards.

## Specs
- MCU: STM32H753ZIT6 @ 480MHz
- HSE: 25MHz
- LSE: 32.768KHz
- Flash: 2MB
- RAM: 512KB

## Usage as Submodule
```
git submodule add https://github.com/WPI-HPRC/mars-board-definitions.git board
git submodule update --init
```
## platformio.ini
```
[platformio]
boards_dir = mars-board-definitions/boards

[env:marsv20]
platform = ststm32
framework = arduino
board = marsv20
board_build.variants_dir = mars-board-definitions/variants
```

### SPI/I2C/Serial Buses
Add this to the top of your `main.cpp`:
```cpp
#include <Wire.h>
#include <HardwareSerial.h>
#include <SPI.h>

SPIClass SENSORS_SPI(SENSORS_SPI_MOSI, SENSORS_SPI_MISO, SENSORS_SPI_SCK);
TwoWire GPS_I2C(GPS_I2C_SDA, GPS_I2C_SCL);
HardwareSerial GPS_SERIAL(GPS_SERIAL_RX, GPS_SERIAL_TX);
TwoWire CONNECTOR_I2C(CONNECTOR_I2C_SDA, CONNECTOR_I2C_SCL);
SPIClass CAMERA_SPI(CAMERA_MOSI, CAMERA_MISO, CAMERA_SCK);
HardwareSerial RADIO_SERIAL(RADIO_SERIAL_RX, RADIO_SERIAL_TX);
```
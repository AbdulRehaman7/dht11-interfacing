DHT11 Sensor Interfacing with STM32 and LPC1768

Introduction
This project demonstrates interfacing the DHT11 temperature and humidity sensor with STM32F401CCU6 (Black Pill) and LPC1768 microcontrollers using Embedded C.

The STM32 implementation uses the STM32 HAL library in STM32CubeIDE, while the LPC1768 implementation uses CMSIS and peripheral registers in Keil uVision.

Hardware Requirements
- STM32F401CCU6 Black Pill
- LPC1768 development board
- DHT11 temperature and humidity sensor
- Jumper wires
- Breadboard
- Pull-up resistor, if required

Software Requirements
- STM32CubeIDE
- Keil uVision
- STM32 and LPC1768 device support packages

Pin Connections

STM32F401CCU6

| DHT11 Pin | STM32 Pin |
|---|---|
| VCC | 3.3V |
| DATA | PA1 |
| GND | GND |

LPC1768

| DHT11 Pin | LPC1768 Pin |
|---|---|
| VCC | 3.3V |
| DATA | P0.0 |
| GND | GND |

Ensure that the DATA line has a suitable pull-up resistor if one is not already present on the sensor module.

Working Principle
The DHT11 uses a capacitive sensing element to measure relative humidity and an NTC thermistor to measure temperature. Its internal controller processes these measurements and transmits digital data to the microcontroller through a single data line.

The communication process consists of:
1. The microcontroller sends a start signal by pulling the DATA line LOW for at least 18 ms.
2. The sensor responds with acknowledgement pulses.
3. The sensor transmits a 40-bit data frame.
4. The microcontroller decodes the received bits based on pulse duration.
5. The checksum is verified to check data integrity.
6. The temperature and humidity values are extracted.

## DHT11 Data Frame

|  Byte    | Description |

| Byte 1 | Integer relative humidity |
| Byte 2 | Decimal relative humidity |
| Byte 3 | Integer temperature |
| Byte 4 | Decimal temperature |
| Byte 5 | Checksum |

The checksum is the lowest 8 bits of the sum of the first four bytes.

STM32 Implementation
- Microcontroller: STM32F401CCU6
- IDE: STM32CubeIDE
- Programming language: Embedded C
- Driver: STM32 HAL
- DATA pin: PA1
- Timing: DWT cycle counter for microsecond delays

The program sends the start signal, receives the sensor data, verifies the checksum, and extracts temperature and humidity readings.

LPC1768 Implementation
- Microcontroller: LPC1768
- IDE: Keil uVision
- Programming language: Embedded C
- Driver: CMSIS and peripheral register-level programming
- DATA pin: P0.0
- Timing: Timer0-based delays

The program configures the GPIO and timer, communicates with the sensor, reads the 40-bit frame, and validates the received data.


How to Run
1. Open the appropriate project in STM32CubeIDE or Keil uVision.
2. Add the corresponding source code.
3. Verify the microcontroller clock configuration and GPIO settings.
4. Connect the DHT11 according to the pin connection table.
5. Build and flash the program.
6. Debug the program and inspect the temperature, humidity, and communication status variables.

Troubleshooting
- Check VCC, GND, and DATA connections.
- Confirm that the configured GPIO pin matches the hardware connection.
- Verify the pull-up resistor on the DATA line.
- Check the clock and timer configuration.
- Verify the checksum before accepting sensor readings.
- Allow at least one second between DHT11 readings.
- If readings fail intermittently, check the timing routines and interrupt effects.

Applications
- Environmental monitoring
- Temperature and humidity measurement
- Embedded systems laboratory experiments
- Sensor interfacing practice
- IoT monitoring prototypes

References
1. DHT11 Technical Data Sheet: https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf
2. STM32F401 Documentation: https://www.st.com/en/microcontrollers-microprocessors/stm32f401/documentation.html
3. LPC1768 Product Documentation: https://www.nxp.com/products/LPC1768

Author
Abdul Rehman

Embedded Systems | Microcontroller Interfacing | Embedded C

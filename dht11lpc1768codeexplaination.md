# DHT11 Interfacing with LPC1768 — Code Explanation

## 1. Introduction

This document explains the Embedded C program used to interface the DHT11 temperature and humidity sensor with the LPC1768 microcontroller.

The program uses CMSIS device definitions and direct peripheral register access to configure GPIO and Timer0. The DHT11 communicates with the microcontroller through a single digital DATA line connected to pin P0.0.

The program performs the following operations:

- Initializes Timer0 for microsecond timing.
- Configures P0.0 as a GPIO output or input.
- Sends the DHT11 start signal.
- Detects the sensor response.
- Receives the 40-bit data frame.
- Verifies the checksum.
- Extracts temperature and humidity values.
- Stores the measurements and communication status in global variables.

## 2. Header Files

```c
#include <LPC17xx.h>
#include <stdint.h>
```

### `LPC17xx.h`

This header file contains register definitions for the LPC1768 microcontroller. It allows the program to access peripherals such as GPIO, Timer0, the system control block, and the pin connection controller.

Examples include:

- `LPC_SC`: System Control registers.
- `LPC_TIM0`: Timer0 registers.
- `LPC_GPIO0`: GPIO Port 0 registers.
- `LPC_PINCON`: Pin connection and configuration registers.

### `stdint.h`

This header provides fixed-width integer types.

- `uint8_t`: An unsigned 8-bit integer.
- `uint32_t`: An unsigned 32-bit integer.

These types are useful for storing sensor bytes, status codes, and timer values.

## 3. DHT11 Pin Configuration

```c
#define DHT_PIN (1U << 0)
```

The expression `1U << 0` shifts the binary value `1` zero positions to the left.

The result is:

```text
00000001
```

This bit mask selects bit 0 of GPIO Port 0, corresponding to pin P0.0.

The program uses this mask when configuring the pin direction, driving the line LOW or releasing it, and reading its logic level.

### Hardware connections

| DHT11 terminal | LPC1768 connection |
|---|---|
| VCC | 3.3 V |
| DATA | P0.0 |
| GND | GND |

A suitable pull-up resistor is required if the sensor module does not already include one.

## 4. Global Variables

```c
volatile uint8_t Temperature = 0;
volatile uint8_t Humidity = 0;
volatile uint8_t DHT_Status = 0;
```

These global variables hold the measurement results and the status of the most recent reading attempt.

### Temperature

Stores the integer temperature value received from the sensor, generally in degrees Celsius.

### Humidity

Stores the integer relative humidity value, expressed as a percentage.

### DHT_Status

Indicates whether the most recent attempt succeeded or failed.

| Value | Meaning |
|---|---|
| 0 | No successful reading yet |
| 1 | Successful reading |
| 2 | Communication error |
| 3 | Checksum error |

The `volatile` keyword helps ensure that the compiler does not assume these variables remain unchanged when they are inspected or accessed outside the compiler's ordinary flow analysis. It is particularly useful when examining the variables in the Keil debugger.

However, `volatile` does not itself provide thread safety or synchronization.

## 5. Timer0 Initialization

### Function: `Timer0_Init()`

The DHT11 requires accurate timing to measure pulses lasting a few tens of microseconds. Timer0 is used to provide a hardware counter for these timing operations.

The function configures Timer0 by accessing the LPC1768 peripheral registers directly.

### 5.1 Enable Timer0 power

```c
LPC_SC->PCONP |= (1U << 1);
```

The `PCONP` register controls peripheral power.

Setting bit 1 enables power to Timer0, allowing the peripheral to operate.

### 5.2 Configure the peripheral clock

```c
LPC_SC->PCLKSEL0 &= ~(3U << 2);
LPC_SC->PCLKSEL0 |= (1U << 2);
```

The `PCLKSEL0` register selects peripheral clock settings.

The first statement clears bits 3 and 2 associated with Timer0's peripheral clock selection.

The second statement sets the selection bits to `01`, which selects a peripheral clock equal to the CPU clock, CCLK, on the LPC1768.

This is a significant clock assumption: the resulting peripheral clock depends on the actual system clock configuration.

### 5.3 Reset Timer0

```c
LPC_TIM0->TCR = 0x02;
```

The Timer Control Register, `TCR`, controls timer operation.

Writing `0x02` sets the reset bit and resets the timer counter and prescaler.

### 5.4 Configure the prescaler

```c
LPC_TIM0->PR = 99;
```

The Prescale Register determines how many peripheral clock cycles occur before the timer counter increments.

The timer counter increments once for every `PR + 1` peripheral clock cycles.

If the Timer0 peripheral clock is 100 MHz:

\[
f_{\text{timer}}=\frac{100\,\text{MHz}}{99+1}
=1\,\text{MHz}
\]

At 1 MHz, the timer counter increments approximately once per microsecond.

**Important:** The value `99` produces a 1 MHz timer tick only when the actual Timer0 peripheral clock is 100 MHz. Verify the system and peripheral clock settings before relying on the delay calculations.

### 5.5 Set the match register

```c
LPC_TIM0->MR0 = 0xFFFFFFFF;
```

The Match Register 0 is assigned its maximum 32-bit value.

The program does not configure match actions, so this setting is not used to generate a periodic interrupt or reset the counter.

### 5.6 Disable match actions

```c
LPC_TIM0->MCR = 0;
```

The Match Control Register determines what happens when the timer counter matches a match register.

Writing zero disables match-related interrupt, reset, and stop actions.

### 5.7 Start Timer0

```c
LPC_TIM0->TCR = 0x01;
```

Setting bit 0 enables the timer counter. Timer0 now begins counting according to its configured clock and prescaler.

## 6. Microsecond Delay

### Function: `delay_us()`

```c
void delay_us(uint32_t us)
{
    uint32_t start = LPC_TIM0->TC;

    while ((uint32_t)(LPC_TIM0->TC - start) < us)
    {
        // Wait
    }
}
```

This function creates a delay by observing the Timer0 counter.

### How it works

1. Read the current timer counter and store it in `start`.
2. Continuously read the current counter.
3. Subtract the starting value from the current value.
4. Exit the loop when the elapsed count reaches the requested number of microseconds.

For example, if the timer increments once per microsecond, `delay_us(30)` waits for approximately 30 timer ticks.

The unsigned subtraction also handles the counter wrapping around from its maximum value to zero, provided the requested delay is within the counter's unambiguous measurement range.

The function assumes that Timer0 has been initialized and that its counter is running.

## 7. Millisecond Delay

### Function: `delay_ms()`

```c
void delay_ms(uint32_t ms)
{
    while (ms--)
    {
        delay_us(1000);
    }
}
```

This function creates longer delays by calling `delay_us(1000)` repeatedly.

Each iteration requests a delay of approximately 1000 microseconds, equivalent to one millisecond when the timer tick is correctly configured.

For example:

- `delay_ms(1)` requests approximately 1 ms.
- `delay_ms(20)` requests approximately 20 ms.
- `delay_ms(1000)` requests approximately 1000 ms, or one second.

The actual delay also includes function-call and loop overhead.

## 8. Configure P0.0 as an Output

### Function: `DHT_Output()`

```c
LPC_PINCON->PINSEL0 &= ~(3U << 0);
LPC_GPIO0->FIODIR |= DHT_PIN;
```

The first statement configures the pin's function selection.

For P0.0, bits 1 and 0 of `PINSEL0` select the pin function. Clearing these bits selects GPIO functionality.

The second statement sets bit 0 in `FIODIR`, configuring P0.0 as an output.

The microcontroller can then drive the DATA line LOW to initiate communication.

**Electrical note:** This implementation uses ordinary push-pull GPIO output mode, not open-drain mode. The sensor and external circuit must be connected so the microcontroller does not actively drive HIGH against the sensor's LOW response. For a robust implementation, use a suitable open-drain interface or release the line appropriately before the sensor responds.

## 9. Configure P0.0 as an Input

### Function: `DHT_Input()`

```c
LPC_PINCON->PINSEL0 &= ~(3U << 0);
LPC_GPIO0->FIODIR &= ~DHT_PIN;
```

The first statement selects GPIO functionality for P0.0.

The second statement clears bit 0 in `FIODIR`, configuring P0.0 as an input.

This allows the microcontroller to observe the logic levels generated on the DATA line during the sensor response and data transmission.

The program switches the pin to input mode after sending the start signal.

## 10. Driving the DATA Line LOW

### Function: `DHT_Low()`

```c
LPC_GPIO0->FIOCLR = DHT_PIN;
```

`FIOCLR` is the Fast GPIO Clear register.

Writing a `1` to the selected bit clears the corresponding GPIO output latch. For P0.0, this requests a LOW output level when the pin is configured as an output.

The function is used to generate the DHT11 start signal.

## 11. Releasing the DATA Line

### Function: `DHT_Release()`

```c
LPC_GPIO0->FIOSET = DHT_PIN;
```

`FIOSET` is the Fast GPIO Set register.

Writing a `1` to the selected bit sets the GPIO output latch for P0.0.

In this program, the function is intended to release the DATA line after the start signal. However, with push-pull output mode, setting the latch drives the pin HIGH rather than electrically releasing it.

The code subsequently switches P0.0 to input mode, which allows the sensor to control the line during communication. A safer implementation should switch to input or use an appropriate open-drain interface before the sensor begins responding.

## 12. Reading the DATA Pin

### Function: `DHT_ReadPin()`

```c
uint8_t DHT_ReadPin(void)
{
    return (LPC_GPIO0->FIOPIN & DHT_PIN) ? 1U : 0U;
}
```

The `FIOPIN` register reflects the current logic level of the GPIO pins.

The expression:

```c
LPC_GPIO0->FIOPIN & DHT_PIN
```

masks all bits except bit 0.

- If P0.0 is HIGH, the expression is nonzero and the function returns `1`.
- If P0.0 is LOW, the expression is zero and the function returns `0`.

This function provides a simple way to read the DHT11 DATA signal.

## 13. Waiting for a Particular Logic Level

### Function: `DHT_WaitLevel()`

This function waits until P0.0 reaches a specified logic level or the timeout expires.

Parameters:

- `level`: `1` for HIGH or `0` for LOW.
- `timeout_us`: maximum time to wait, assuming one timer tick per microsecond.

### Operation

1. Record the starting Timer0 count.
2. Read the pin repeatedly.
3. Compare the current logic level with the requested level.
4. If the timer difference reaches the timeout, return `0`.
5. If the requested level is detected, return `1`.

The timeout is important because it prevents the program from waiting indefinitely if the sensor does not respond.

## 14. Reading One Byte

### Function: `DHT_ReadByte()`

The DHT11 transmits each byte with the most significant bit first.

Each data bit begins with a LOW interval and then a HIGH pulse. The duration of the HIGH pulse distinguishes a binary 0 from a binary 1.

Typical DHT11 timing is approximately:

- Binary 0: HIGH for about 26–28 microseconds.
- Binary 1: HIGH for about 70 microseconds.

The code uses 45 microseconds as its threshold.

### Operation

1. Wait for the DATA line to become HIGH.
2. Record the current Timer0 counter.
3. Wait until the DATA line returns LOW.
4. Calculate the elapsed timer ticks.
5. Shift the previously received bits left by one position.
6. If the HIGH duration exceeds 45 ticks, set the least-significant bit to `1`.
7. Otherwise, leave that bit as `0`.
8. Repeat until eight bits have been collected.

For example, receiving the bits `00000101` produces the decimal value `5`.

The completed byte is stored through the pointer `value`.

If the required pin transition does not occur before its timeout, the function returns `0`. If all eight bits are received, it returns `1`.

## 15. Reading the Complete DHT11 Frame

### Function: `DHT_Read()`

This function manages the complete communication sequence.

It returns:

- `1` when communication and checksum verification succeed.
- `0` when a communication timeout occurs.
- `2` when the checksum does not match.

### 15.1 Allocate the data array

```c
uint8_t data[5] = {0};
```

The DHT11 transmits 40 bits, which are stored as five 8-bit values.

### 15.2 Send the start signal

```c
DHT_Output();
DHT_Low();
delay_ms(20);
```

The pin is configured as an output and pulled LOW for 20 milliseconds.

This signals the DHT11 that the microcontroller wants to begin communication.

### 15.3 Release the line and change to input mode

```c
DHT_Release();
delay_us(30);
DHT_Input();
```

The output latch is set, a short delay is introduced, and P0.0 is configured as an input.

The input configuration allows the sensor to generate the response and transmit the data. As noted earlier, the pin should be released safely before the sensor begins driving the line.

### 15.4 Detect the sensor response

The code checks the following sequence:

```c
if (!DHT_WaitLevel(0, 100))
    return 0;

if (!DHT_WaitLevel(1, 100))
    return 0;

if (!DHT_WaitLevel(0, 100))
    return 0;
```

The expected response contains a LOW pulse, followed by a HIGH pulse, and then a LOW interval before data transmission.

The `!` operator reverses the function's return value for the condition. If `DHT_WaitLevel()` returns `0`, the program immediately returns a communication error.

### 15.5 Receive five bytes

```c
for (i = 0; i < 5; i++)
{
    if (!DHT_ReadByte(&data[i]))
        return 0;
}
```

The loop calls `DHT_ReadByte()` five times.

Each call receives eight bits, giving:

40 bits = 5 bytes × 8 bits.

The address expression `&data[i]` passes the location where the received byte should be stored.

### 15.6 Calculate the checksum

```c
uint8_t checksum =
    (uint8_t)(data[0] + data[1] + data[2] + data[3]);
```

The checksum is calculated by adding the first four bytes and retaining the lowest eight bits of the result.

The fifth byte, `data[4]`, contains the checksum sent by the sensor.

### 15.7 Verify the checksum

```c
if (checksum != data[4])
    return 2;
```

If the calculated checksum differs from the received checksum, the function returns `2`.

This indicates that the data frame failed its checksum check and should not be accepted as a valid measurement.

### 15.8 Extract the readings

```c
*hum = data[0];
*temp = data[2];
```

The first byte contains the integer part of relative humidity, and the third byte contains the integer part of temperature.

The values are written to the memory locations supplied through the `hum` and `temp` pointers.

The decimal bytes are not used by this implementation. After extracting the values, the function returns `1` to indicate success.

## 16. Main Function

### Function: `main()`

The main function initializes Timer0 and the DATA pin before entering an infinite loop.

### 16.1 Initialize Timer0

```c
Timer0_Init();
```

Configures Timer0 and starts its counter. The delay and timeout functions rely on this counter.

### 16.2 Configure the DATA pin

```c
DHT_Output();
DHT_Release();
```

The pin is configured as an output and the output latch is set HIGH.

For safe operation, ensure that the electrical interface releases the DATA line before the sensor responds.

### 16.3 Enter the infinite loop

```c
while (1)
```

The microcontroller repeatedly requests new temperature and humidity measurements.

Inside each iteration, the program creates temporary variables `t` and `h`, then calls `DHT_Read()`.

### 16.4 Handle a successful reading

```c
if (result == 1)
{
    Temperature = t;
    Humidity = h;
    DHT_Status = 1;
}
```

If the sensor reading succeeds and the checksum is valid, the program updates the global temperature and humidity variables and records a successful status.

### 16.5 Handle a checksum error

```c
else if (result == 2)
{
    DHT_Status = 3;
}
```

If the checksum is invalid, the program records status `3`.

The previous temperature and humidity values are retained.

### 16.6 Handle a communication error

```c
else
{
    DHT_Status = 2;
}
```

If the sensor does not respond correctly or a timeout occurs, the program records status `2`.

The previous valid measurement values remain unchanged.

### 16.7 Wait before the next reading

```c
delay_ms(1000);
```

The program waits approximately one second before requesting another measurement. This is consistent with the recommended minimum reading interval for the DHT11.

## 17. Overall Program Flow

1. Initialize Timer0.
2. Configure P0.0.
3. Send a 20 ms LOW start signal.
4. Release the DATA line and switch the pin to input mode.
5. Detect the sensor response.
6. Read five bytes from the sensor.
7. Calculate and verify the checksum.
8. Extract integer temperature and humidity values.
9. Update global variables and status.
10. Wait approximately one second.
11. Repeat the process.

## 18. Important Implementation Considerations

### Timer clock accuracy

The prescaler value `99` assumes a 100 MHz Timer0 peripheral clock. If the actual peripheral clock differs, `delay_us()` and the pulse timeout calculations will not represent the intended time intervals.

### GPIO electrical behavior

The code uses ordinary GPIO output mode. The DATA line must not be driven HIGH by the STM32 while the sensor is trying to pull it LOW. Use a suitable open-drain interface or switch the pin to input mode at the correct point in the protocol.

### Timing reliability

Polling loops can be affected by software overhead and interrupts. For improved reliability, consider using hardware timer input capture or another timing method validated against the actual clock configuration.

### Measurement validity

The program retains the last valid temperature and humidity readings if a new reading fails. Always check `DHT_Status` before interpreting the stored values as the latest measurement.

### Data format

This implementation extracts only integer temperature and humidity values. It does not use the decimal bytes or implement special handling for negative temperature readings.

## 19. Conclusion

This project demonstrates DHT11 sensor interfacing with the LPC1768 using Embedded C and direct peripheral register access.

It covers Timer0 initialisation, GPIO configuration, microsecond delays, pulse-width decoding, checksum verification, and error handling. The implementation provides a foundation for understanding digital sensor communication and can be extended with more robust timing and electrical-interface techniques.



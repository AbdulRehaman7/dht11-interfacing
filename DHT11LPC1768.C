#include <LPC17xx.h>
#include <stdint.h>
#define DHT_PIN     (1U << 0)/* DHT11 DATA pin: P0.0 */
volatile uint8_t Temperature = 0;/* Global variables for debugging */
volatile uint8_t Humidity = 0;
volatile uint8_t DHT_Status = 0;

/*
DHT_Status:
0 = No successful reading yet
1 = Successful reading
2 = Communication error
3 = Checksum error
*/

/* Initialize Timer0 for microsecond timing */
void Timer0_Init(void)
{
    LPC_SC->PCONP |= (1U << 1);      // Enable Timer0 power

    LPC_SC->PCLKSEL0 &= ~(3U << 2);  // Clear Timer0 peripheral clock bits
    LPC_SC->PCLKSEL0 |=  (1U << 2);  // Timer0 clock = CCLK

    LPC_TIM0->TCR = 0x02;            // Reset Timer0

    LPC_TIM0->PR = 99;               // 100 MHz / 100 = 1 MHz timer tick

    LPC_TIM0->MR0 = 0xFFFFFFFF;      // Set a large match value

    LPC_TIM0->MCR = 0;               // Disable match-control actions

    LPC_TIM0->TCR = 0x01;            // Start Timer0
}

/* Delay for the specified number of microseconds */
void delay_us(uint32_t us)
{
    uint32_t start = LPC_TIM0->TC;   // Record the starting timer count

    while ((uint32_t)(LPC_TIM0->TC - start) < us)
    {
        // Wait until the requested time has elapsed
    }
}

/* Delay in milliseconds */
void delay_ms(uint32_t ms)
{
    while (ms--)
    {
        delay_us(1000);              // Delay for 1 millisecond
    }
}

/* Configure P0.0 as GPIO output */
void DHT_Output(void)
{
    LPC_PINCON->PINSEL0 &= ~(3U << 0); // Select GPIO function for P0.0
    LPC_GPIO0->FIODIR |= DHT_PIN;      // Configure P0.0 as output
}

/* Configure P0.0 as GPIO input */
void DHT_Input(void)
{
    LPC_PINCON->PINSEL0 &= ~(3U << 0); // Select GPIO function for P0.0
    LPC_GPIO0->FIODIR &= ~DHT_PIN;     // Configure P0.0 as input
}

/* Drive the DATA pin LOW */
void DHT_Low(void)
{
    LPC_GPIO0->FIOCLR = DHT_PIN;       // Clear P0.0 output
}

/* Release the DATA pin HIGH */
void DHT_Release(void)
{
    LPC_GPIO0->FIOSET = DHT_PIN;       // Set P0.0 output latch
}

/* Read the DATA pin */
uint8_t DHT_ReadPin(void)
{
    return (LPC_GPIO0->FIOPIN & DHT_PIN) ? 1U : 0U;
}

/* Wait for a pin level, with timeout in microseconds */
uint8_t DHT_WaitLevel(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = LPC_TIM0->TC;

    while (DHT_ReadPin() != level)
    {
        if ((uint32_t)(LPC_TIM0->TC - start) >= timeout_us)
        {
            return 0;                // Timeout
        }
    }

    return 1;                        // Requested level detected
}

/* Read one byte from the DHT11 */
uint8_t DHT_ReadByte(uint8_t *value)
{
    uint8_t data = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        /* Wait for the DATA line to become HIGH */
        if (!DHT_WaitLevel(1, 100))
            return 0;

        /* Measure the duration of the HIGH pulse */
        uint32_t start = LPC_TIM0->TC;

        /* Wait for the HIGH pulse to end */
        if (!DHT_WaitLevel(0, 120))
            return 0;

        uint32_t elapsed = LPC_TIM0->TC - start;

        /* Shift left to make space for the next bit */
        data <<= 1;

        /* A HIGH pulse longer than 45 us represents binary 1 */
        if (elapsed > 45)
        {
            data |= 1U;
        }
    }

    *value = data;                    // Store the received byte

    return 1;                         // Byte received successfully
}

/* Read the complete 40-bit DHT11 data frame */
uint8_t DHT_Read(uint8_t *temp, uint8_t *hum)
{
    uint8_t data[5] = {0};
    uint8_t i;
    DHT_Output(); /* Configure DATA as output */
    DHT_Low(); /* Send start signal */
    delay_ms(20);
    DHT_Release(); /* Release the bus and switch to input */
    delay_us(30);
    DHT_Input();
    if (!DHT_WaitLevel(0, 100))/* Receive the sensor response */
        return 0;

    if (!DHT_WaitLevel(1, 100))
        return 0;

    if (!DHT_WaitLevel(0, 100))
        return 0;

    /* Receive all five bytes */
    for (i = 0; i < 5; i++)
    {
        if (!DHT_ReadByte(&data[i]))
            return 0;
    }
    /* Calculate the checksum */
    uint8_t checksum =(uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4])/* Verify the received checksum */
        return 2;
    *hum = data[0];/* Extract integer humidity and temperature */
    *temp = data[2];

    return 1;
}

/* Main program */
int main(void)
{
    Timer0_Init(); /* Initialize Timer0 */
    DHT_Output();/* Configure P0.0 as output initially */
    DHT_Release(); /* Release the DATA line */
    while (1)
    {
        uint8_t t = 0;
        uint8_t h = 0;
        uint8_t result = DHT_Read(&t, &h);/* Read temperature and humidity */
        if (result == 1)
        {
            Temperature = t;           // Store temperature
            Humidity = h;               // Store humidity
            DHT_Status = 1;             // Reading successful
        }
        else if (result == 2)
        {
            DHT_Status = 3;             // Checksum error
        }
        else
        {
            DHT_Status = 2;             // Communication error
        }

        /* Wait before the next measurement */
        delay_ms(1000);
    }
}
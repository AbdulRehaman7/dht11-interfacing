


#include "main.h"
#include <stdint.h>
#define DHT_PORT GPIOA
#define DHT_PIN  GPIO_PIN_1

volatile uint8_t Temperature = 0;
volatile uint8_t Humidity = 0;
volatile uint8_t DHT_Status = 0;

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles =
        (HAL_RCC_GetHCLKFreq() / 1000000U) * us;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
    }
}

static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* ------------ Configure DATA as Output ------------ */

static void DHT_Output(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = DHT_PIN;                 /* Select PA1 */
    gpio.Mode = GPIO_MODE_OUTPUT_OD;    /* Open-drain output */
    gpio.Pull = GPIO_NOPULL;            /* External pull-up used */
    gpio.Speed = GPIO_SPEED_FREQ_LOW;   /* Low output speed */

    HAL_GPIO_Init(DHT_PORT, &gpio);     /* Apply configuration */
}

/* ------------ Configure DATA as Input ------------ */

static void DHT_Input(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = DHT_PIN;              /* Select PA1 */
    gpio.Mode = GPIO_MODE_INPUT;     /* Receive sensor data */
    gpio.Pull = GPIO_NOPULL;         /* External pull-up used */

    HAL_GPIO_Init(DHT_PORT, &gpio);  /* Apply configuration */
}

/* ------------ Wait for a GPIO Level ------------ */

/* Returns 1 if the requested level is detected, otherwise 0 */
static uint8_t Wait_Pin(GPIO_PinState state,
                        uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;

    /* Calculate the timeout in CPU cycles */
    uint32_t limit =
        (HAL_RCC_GetHCLKFreq() / 1000000U) * timeout_us;

    /* Wait while the pin does not match the requested level */
    while (HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) != state)
    {
        /* Stop waiting if the timeout expires */
        if ((uint32_t)(DWT->CYCCNT - start) >= limit)
        {
            return 0;
        }
    }

    return 1;
}

/* ------------ Read One Byte ------------ */

/* Reads eight bits, with the most significant bit first */
static uint8_t Read_Byte(uint8_t *value)
{
    uint8_t data = 0;

    /* Each byte contains eight bits */
    for (uint8_t i = 0; i < 8; i++)
    {
        /* Wait for the DATA line to become HIGH */
        if (!Wait_Pin(GPIO_PIN_SET, 100))
        {
            return 0;
        }

        /* Record the start of the HIGH pulse */
        uint32_t start = DWT->CYCCNT;

        /* Wait until the HIGH pulse ends */
        if (!Wait_Pin(GPIO_PIN_RESET, 120))
        {
            return 0;
        }

        /* Measure the HIGH pulse duration */
        uint32_t elapsed = DWT->CYCCNT - start;

        /* A 45 us threshold distinguishes 0 from 1 */
        uint32_t threshold =
            (HAL_RCC_GetHCLKFreq() / 1000000U) * 45U;

        /* Shift previous bits left to make space for the next bit */
        data <<= 1;

        /* A longer HIGH pulse represents binary 1 */
        if (elapsed > threshold)
        {
            data |= 1U;
        }
        /* A shorter HIGH pulse represents binary 0 */
    }

    /* Store the received byte */
    *value = data;

    return 1;
}

/* ------------ Read Complete DHT11 Frame ------------ */
/*
 * Return values:
 * 1 = Successful reading
 * 0 = Communication error
 * 2 = Checksum error
 */

static uint8_t DHT_Read(uint8_t *temp, uint8_t *hum)
{
    uint8_t data[5] = {0};/* Five bytes make up the 40-bit data frame */
    DHT_Output();/* Configure DATA pin as output */
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_RESET);/* Send start signal: pull DATA LOW */
    HAL_Delay(20);/* Hold LOW for at least 18 ms */
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);/* Release DATA line HIGH */
    delay_us(30);/* Allow the sensor to respond */
    DHT_Input();/* Change DATA pin to input */ 
    if (!Wait_Pin(GPIO_PIN_RESET, 100))/* Sensor response begins with LOW */
    {
        return 0;
    }
    if (!Wait_Pin(GPIO_PIN_SET, 100))/* Sensor response then becomes HIGH */
    {
        return 0;
    }
    if (!Wait_Pin(GPIO_PIN_RESET, 100))/* Response ends with LOW before data transmission */
    {
        return 0;
    }
    for (uint8_t i = 0; i < 5; i++) /* Read the five bytes */
    {
        if (!Read_Byte(&data[i]))
        {
            return 0;
        }
    }
    /* Calculate checksum from the first four bytes */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4])/* Compare calculated checksum with the fifth byte */
    {
        return 2;
    }
    *hum = data[0];/* Byte 1 contains integral relative humidity */
    *temp = data[2];/* Byte 3 contains integral temperature */
    return 1;
}

/* ------------ Main Function ------------ */

int main(void)
{
   
    HAL_Init(); /* Initialize HAL, including SysTick */
    SystemClock_Config(); /* Configure system clock (generated by CubeMX) */
    MX_GPIO_Init(); /* Initialize GPIO peripherals (generated by CubeMX) */
    DWT_Init(); /* Enable microsecond timing */
    DHT_Output(); /* Initially configure DATA as output */
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);/* Release the open-drain DATA line */
    while (1)
    {
        uint8_t t = 0;
        uint8_t h = 0;
        uint8_t result = DHT_Read(&t, &h); /* Request and receive temperature and humidity */
        if (result == 1)
        {
            Temperature = t; /* Update global variables after a valid reading */
            Humidity = h;
            DHT_Status = 1; /* Indicate successful communication */
        }
        else if (result == 2)
        {
            DHT_Status = 3;/* Checksum does not match */
        }
        else
        {
            
            DHT_Status = 2; /* Sensor did not respond correctly */
        }
        HAL_Delay(1000); /* Wait one second before the next measurement */
    }
}



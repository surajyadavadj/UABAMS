#include "stm32f4xx.h"
#include "usart_debug.h"
#include <stdio.h>
#include "sdio.h"
#define NO_RESP 0
#define SHORT_RESP 1

void SDIO_Init(void)
{
    // Enable clocks
    RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN;

    // PC8–PC12 (D0–D3, CLK)
    for(int i=8;i<=12;i++)
    {
        GPIOC->MODER &= ~(3 << (i*2));
        GPIOC->MODER |=  (2 << (i*2));

        GPIOC->AFR[1] &= ~(0xF << ((i-8)*4));
        GPIOC->AFR[1] |=  (12 << ((i-8)*4));
    }

    // PD2 (CMD)
    GPIOD->MODER &= ~(3 << (2*2));
    GPIOD->MODER |=  (2 << (2*2));
    GPIOD->AFR[0] &= ~(0xF << (2*4));
    GPIOD->AFR[0] |=  (12 << (2*4));

    // Power ON
    SDIO->POWER = 3;

    // Slow clock (~400kHz)
    SDIO->CLKCR = (180 << 0);
    SDIO->CLKCR |= (1 << 8); // enable clock

    usart_debug("SDIO INIT DONE\r\n");
}

int SDIO_SendCMD_Debug(uint8_t cmd, uint32_t arg, uint8_t resp)
{
    char buf[100];

    SDIO->ICR = 0xFFFFFFFF;

    SDIO->ARG = arg;

    if(resp == 0)
        SDIO->CMD = (cmd & 0x3F) | (1 << 10);
    else
        SDIO->CMD = (cmd & 0x3F) | (1 << 10) | (1 << 6);

    uint32_t timeout = 1000000;

    while(!(SDIO->STA & (1 << 6)) &&
          !(SDIO->STA & (1 << 2)) &&
          !(SDIO->STA & (1 << 1)))
    {
        if(--timeout == 0)
        {
            sprintf(buf, "CMD%d TIMEOUT\r\n", cmd);
            usart_debug(buf);
            return 0;
        }
    }

    sprintf(buf, "CMD%d STA: 0x%08lX\r\n", cmd, SDIO->STA);
    usart_debug(buf);

    if(SDIO->STA & (1 << 1))
    {
        usart_debug("TIMEOUT FLAG\r\n");
        return 0;
    }

    if(SDIO->STA & (1 << 2))
    {
        usart_debug("CRC FAIL\r\n");
        return 0;
    }

    sprintf(buf, "RESP1: 0x%08lX\r\n", SDIO->RESP1);
    usart_debug(buf);

    usart_debug("CMD OK\r\n");

    return 1;
}

void SDIO_FullInit_Debug(void)
{
    usart_debug("\r\n==== SD INIT START ====\r\n");

    // CMD0
    usart_debug("Sending CMD0...\r\n");
    if(!SDIO_SendCMD_Debug(0, 0, 0)) return;

    // CMD8
    usart_debug("Sending CMD8...\r\n");
    if(!SDIO_SendCMD_Debug(8, 0x1AA, 1)) return;

    // ACMD41 loop
    uint32_t retry = 0;

    while(1)
    {
        usart_debug("Sending CMD55...\r\n");
        if(!SDIO_SendCMD_Debug(55, 0, 1)) return;

        usart_debug("Sending ACMD41...\r\n");
        if(!SDIO_SendCMD_Debug(41, 0x40000000, 1)) return;

        uint32_t resp = SDIO->RESP1;

        if(resp & (1 << 31))
        {
            usart_debug("CARD READY\r\n");
            break;
        }

        retry++;
        if(retry > 1000)
        {
            usart_debug("ACMD41 TIMEOUT\r\n");
            return;
        }
    }

    usart_debug("==== SD INIT DONE ====\r\n");
}
int SDIO_SendCMD(uint8_t cmd, uint32_t arg, uint8_t resp)
{
    SDIO->ICR = 0xFFFFFFFF;

    SDIO->ARG = arg;

    if(resp == NO_RESP)
        SDIO->CMD = (cmd & 0x3F) | (1 << 10);
    else
        SDIO->CMD = (cmd & 0x3F) | (1 << 10) | (1 << 6);

    uint32_t timeout = 1000000;

    while(!(SDIO->STA & (1 << 7)) &&  // CMDSENT
          !(SDIO->STA & (1 << 6)) &&  // CMDREND
          !(SDIO->STA & (1 << 2)) &&
          !(SDIO->STA & (1 << 1)))
    {
        if(--timeout == 0)
        {
            usart_debug("CMD TIMEOUT\r\n");
            return 0;
        }
    }

    if(SDIO->STA & (1 << 1)) return 0;
    if(SDIO->STA & (1 << 2)) return 0;

    return 1;
}
int SDIO_CardInit(void)
{
    usart_debug("\r\n--- SD INIT START ---\r\n");

    // CMD0 → Reset
    if(!SDIO_SendCMD(0, 0, 0))
    {
        usart_debug("CMD0 FAIL\r\n");
        return 0;
    }
    usart_debug("CMD0 OK\r\n");

    // CMD8 → Voltage check
    if(!SDIO_SendCMD(8, 0x1AA, 1))
    {
        usart_debug("CMD8 FAIL\r\n");
        return 0;
    }
    usart_debug("CMD8 OK\r\n");

    // ACMD41 loop
    uint32_t retry = 0;

    while(1)
    {
        // CMD55
        if(!SDIO_SendCMD(55, 0, 1))
        {
            usart_debug("CMD55 FAIL\r\n");
            return 0;
        }

        // ACMD41
        if(!SDIO_SendCMD(41, 0x40000000, 1))
        {
            usart_debug("ACMD41 FAIL\r\n");
            return 0;
        }

        // Check ready (OCR register)
        uint32_t resp = SDIO->RESP1;

        if(resp & (1 << 31))  // ready bit
        {
            usart_debug("CARD READY\r\n");
            break;
        }

        retry++;
        if(retry > 1000)
        {
            usart_debug("ACMD41 TIMEOUT\r\n");
            return 0;
        }
    }

    usart_debug("--- SD INIT DONE ---\r\n");
    return 1;
}
void SDIO_Test(void)
{
    SDIO_Init();

    usart_debug("SDIO INIT DONE\r\n");

    // CMD0
    usart_debug("Sending CMD0...\r\n");
    SDIO_SendCMD(0, 0, NO_RESP);

    // CMD8
    usart_debug("Sending CMD8...\r\n");

    if(SDIO_SendCMD(8, 0x1AA, SHORT_RESP))
    {
        uint32_t resp = SDIO->RESP1;

        char buf[50];
        sprintf(buf, "RESP1: 0x%08lX\r\n", resp);
        usart_debug(buf);

        if(resp == 0x1AA)
            usart_debug("CARD DETECTED \r\n");
        else
            usart_debug("INVALID RESPONSE \r\n");
    }
    else
    {
        usart_debug("NO RESPONSE \r\n");
    }
}

void SDIO_PinTest(void)
{
    usart_debug("\r\n=== PIN TEST START ===\r\n");

    // CLK toggle test (PC12)
    GPIOC->MODER &= ~(3 << (12*2));
    GPIOC->MODER |=  (1 << (12*2)); // output

    for(int i=0; i<10; i++)
    {
        GPIOC->ODR ^= (1 << 12);
        for(volatile int d=0; d<500000; d++);
    }

    usart_debug("CLK TOGGLE DONE\r\n");

    // CMD read test (PD2)
    GPIOD->MODER &= ~(3 << (2*2)); // input

    if(GPIOD->IDR & (1 << 2))
        usart_debug("CMD LINE HIGH (OK)\r\n");
    else
        usart_debug("CMD LINE LOW (PROBLEM)\r\n");
}

void SDIO_ClockTest(void)
{
    usart_debug("\r\n=== CLOCK TEST ===\r\n");

    SDIO->POWER = 3;

    SDIO->CLKCR = (255 << 0); // slow
    SDIO->CLKCR |= (1 << 8);  // enable

    usart_debug("Clock Enabled\r\n");
}
void SDIO_CMD0_Test(void)
{
    usart_debug("\r\n=== CMD0 TEST ===\r\n");

    SDIO->ICR = 0xFFFFFFFF;

    SDIO->ARG = 0;
    SDIO->CMD = (0 << 0) | (1 << 10); // NO RESP

    uint32_t timeout = 1000000;

    while(!(SDIO->STA & (1 << 7))) // CMDSENT
    {
        if(--timeout == 0)
        {
            usart_debug("CMD0 NOT SENT \r\n");
            return;
        }
    }

    usart_debug("CMD0 SENT OK ✅\r\n");
}
void SDIO_CMD8_Test(void)
{
    usart_debug("\r\n=== CMD8 TEST ===\r\n");

    SDIO->ICR = 0xFFFFFFFF;

    SDIO->ARG = 0x1AA;
    SDIO->CMD = (8 << 0)  | (1 << 6)| (1 << 10);

    uint32_t timeout = 1000000;

    while(!(SDIO->STA & (1 << 6)) &&
          !(SDIO->STA & (1 << 1)))
    {
        if(--timeout == 0)
        {
            usart_debug("CMD8 NO RESPONSE \r\n");
            return;
        }
    }

    char buf[50];
    sprintf(buf, "STA: 0x%08lX\r\n", SDIO->STA);
    usart_debug(buf);

    sprintf(buf, "RESP1: 0x%08lX\r\n", SDIO->RESP1);
    usart_debug(buf);

    usart_debug("CMD8 DONE\r\n");
}


void SD_CardDetect_Test(void)
{
    usart_debug("\r\n=== SD CARD DETECT TEST ===\r\n");

    // CMD (PD2) → input
    GPIOD->MODER &= ~(3 << (2*2));

    // D0 (PC8) → input
    GPIOC->MODER &= ~(3 << (8*2));

    int cmd = (GPIOD->IDR & (1 << 2)) ? 1 : 0;
    int d0  = (GPIOC->IDR & (1 << 8)) ? 1 : 0;

    uint32_t resp = SDIO->RESP1;

if(resp == 0x1AA)
{
    usart_debug("CARD PRESENT \r\n");
}
else
{
    usart_debug("NO CARD \r\n");
}

char buf[50];
//int cmd, d0;

 cmd = (GPIOD->IDR & (1 << 2)) ? 1 : 0;
 d0  = (GPIOC->IDR & (1 << 8)) ? 1 : 0;

sprintf(buf, "RAW PIN STATE → CMD=%d, D0=%d\r\n", cmd, d0);
usart_debug(buf);

if(cmd == 0 || d0 == 0)
{
    usart_debug(" SIGNAL ISSUE (LINE LOW)\r\n");
}
else
{
    usart_debug("LINES HIGH (NORMAL)\r\n");
}
}


void SDIO_DebugFull(void)
{
    usart_debug("\r\n===== SDIO FULL DEBUG =====\r\n");
     SD_CardDetect_Test(); 

    SDIO_PinTest();     
    SDIO_ClockTest();
    SDIO_CMD0_Test();
    SDIO_CMD8_Test();
}
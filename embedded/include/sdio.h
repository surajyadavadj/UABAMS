#ifndef SDIO_H
#define SDIO_H


void SDIO_Init(void);
void SDIO_Test(void);
void SDIO_FullInit_Debug(void);
int SDIO_SendCMD_Debug(uint8_t cmd, uint32_t arg, uint8_t resp);
void SDIO_DebugFull(void);

#endif
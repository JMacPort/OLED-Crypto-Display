#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "font_5x7.h"

// Prototypes
void USART2_Init();
void USART2_Print(const char*, ...);
void I2C_Init();
void OLED_Init();
void OLED_WriteString(const char*);
void OLED_Clear();
void OLED_SetCursor(uint8_t, uint8_t);
void USART1_Init();
void ESP_Send_Command(const char*);
void ESP_Response(char*);
char Wifi_Get_Char();
void Wifi_Read_Response_With_Timeout();
void Wifi_Send_Command(const char*);
void Wifi_HTTPS_GET();
void Timer2_Init();
//void find_addr();

// OLED
#define OLED_ADDR					0x3C

#define MAX_BUFFER					500

char buffer[MAX_BUFFER];

int main() {
	USART1_Init();

	USART2_Init();
	USART2_Print("Ready....\r\n");

	I2C_Init();

	OLED_Init();
	Timer2_Init();



	while (1) {
	}
}

void USART2_Init() {
	RCC -> APB1ENR |= (1 << 17);										// USART2 Clock
	RCC -> AHB1ENR |= (1 << 0);											// GPIOA Clock

	GPIOA -> MODER &= ~((3 << (2 * 2)) | (3 << (2 * 3)));				// Sets mode to Alternate Function
	GPIOA -> MODER |= (2 << (2 * 2)) | (2 << (2 * 3));

	GPIOA -> AFR[0] |= (7 << (4 * 2)) | (7 << (4 * 3));					// AFR set to USART2

	USART2 -> BRR = 16000000 / 115200;									// Baud Rate 115200
	USART2 -> CR1 |= (1 << 3) | (1 << 13);								// Transmit Enable; USART2 Enable
}

void USART2_Print(const char* str, ...) {
    char buffer[100];

    va_list args;

    va_start(args, str);
    vsprintf(buffer, str, args);
    va_end(args);

    char* ptr = buffer;
    while(*ptr) {
        while(!(USART2->SR & (1 << 7)));
        USART2->DR = *ptr++;
    }
}

void I2C_Init() {
	RCC -> APB1ENR |= (1 << 21); 										// I2C Clock
	RCC -> AHB1ENR |= (1 << 1);											// GPIOB Clock

	GPIOB -> MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2))); 				// Sets mode to Alternate Function
	GPIOB -> MODER |= (2 << (8 * 2)) | (2 << (9 *2));

	GPIOB -> OTYPER |= (1 << 8) | (1 << 9);								// Open-Drain for PB8 and PB9

	GPIOB -> PUPDR &= ~((3 << (8 * 2)) | (3 << (9 * 2)));				// Sets pins to Pull-Up
	GPIOB -> PUPDR |= (1 << (8 * 2)) | (1 << (9 * 2));

	GPIOB -> AFR[1] &= ~(0xF << 0) | (0xF << 4);						// Sets the Alternate Function to AF4 for PB8 and PB9
	GPIOB -> AFR[1] |= (4 << 0) | (4 << 4);

	I2C1 -> CR1 |= (1 << 15);											// Toggles SWRST; Resets the I2C connection
	I2C1 -> CR1 &= ~(1 << 15);

	I2C1 -> CR2 |= (42 << 0);											// Sets clock frequency to 42MHz
	I2C1 -> CCR = 210;													// Configured for 100kHz; Standard frequency
	I2C1 -> TRISE = 43;													// Frequency + 1; Used to set maximum rise time

	I2C1 -> CR1 |= (1 << 0);											// Enables I2C
}

uint8_t I2C_CheckBusy() {
	if (I2C1 -> SR1 & (1 << 1)) {
		return 1;
	}
	return 0;
}


void I2C_Start() {
	I2C1 -> CR1 |= (1 << 8);

	while (!(I2C1 -> SR1 & (1 << 0)));
}


void I2C_SendAddress(uint8_t address, uint8_t read) {
	I2C1 -> DR = (address << 1) | read;

	uint32_t timeout = 10000;
	while(!(I2C1->SR1 & (1 << 1)) && timeout) timeout--;
	if(timeout == 0) {
	   USART2_Print("Address timeout\r\n");
	}

	uint8_t dummyRead = I2C1 -> SR1;
	dummyRead = I2C1 -> SR2;
	(void)dummyRead;
}

void I2C_SendData(uint8_t data) {
	while (!(I2C1 -> SR1 & (1 << 7)));

	I2C1 -> DR = data;

	while (!(I2C1 -> SR1 & (1 << 2)));
}

void I2C_Stop() {
	I2C1 -> CR1 |= (1 << 9);
}

void I2C_Write(uint8_t address, uint8_t data) {
	I2C_CheckBusy();
	I2C_Start();
	I2C_SendAddress(address, 0);
	I2C_SendData(data);
	I2C_Stop();
}

void OLED_Send_Command(uint8_t cmd) {
    I2C_Start();
    I2C_SendAddress(OLED_ADDR, 0);
    I2C_SendData(0x00);
    I2C_SendData(cmd);
    I2C_Stop();
}


// Possibly can optimize this a bit more to expand functionality to only clear used rows?
void OLED_Clear() {
    OLED_Send_Command(0xAE);    // Display off

    OLED_Send_Command(0x20);    // Set Memory Addressing Mode
    OLED_Send_Command(0x00);    // Horizontal Addressing Mode

    OLED_Send_Command(0x21);    // Set Column Address
    OLED_Send_Command(0x00);    // Start at column 0
    OLED_Send_Command(0x7F);    // End at column 127

    OLED_Send_Command(0x22);    // Set Page Address
    OLED_Send_Command(0x00);    // Start at page 0
    OLED_Send_Command(0x07);    // End at page 7

    for (uint16_t i = 0; i < 1024; i++) {
        I2C_Start();
        I2C_SendAddress(OLED_ADDR, 0);
        I2C_SendData(0x40);     // Data mode
        I2C_SendData(0x00);     // Clear pixel
        I2C_Stop();
    }

    OLED_Send_Command(0xAF);    // Display on
}

void OLED_Init() {
    OLED_Send_Command(0xAE); // Display OFF

    OLED_Send_Command(0x8D); // Enable Charge Pump
    OLED_Send_Command(0x14);

    OLED_Send_Command(0xA8); // Set Multiplex Ratio
    OLED_Send_Command(0x3F); // 64 lines

    OLED_Send_Command(0xD3); // Set Display Offset
    OLED_Send_Command(0x00); // No offset

    OLED_Send_Command(0x40); // Start line = 0

    OLED_Send_Command(0xA1); // Segment re-map
    OLED_Send_Command(0xC8); // COM scan direction

    OLED_Send_Command(0xA6); // Normal Display (not inverted)

    OLED_Send_Command(0xAF); // Display ON

    OLED_Clear();
}

void OLED_SetCursor(uint8_t col, uint8_t page) {
    OLED_Send_Command(0x21);
    OLED_Send_Command(col);
    OLED_Send_Command(127);

    OLED_Send_Command(0x22);
    OLED_Send_Command(page);
    OLED_Send_Command(7);
}

// Displays one character from the bitmap
void OLED_WriteChar(char c) {
    if (c < 32 || c > 127) c = 32;

    const uint8_t* charData = font_5x7[c - 32];

    for (int i = 0; i < 5; i++) {
        I2C_Start();
        I2C_SendAddress(OLED_ADDR, 0);
        I2C_SendData(0x40);
        I2C_SendData(charData[i]);
        I2C_Stop();
    }
}

// Displays a string on OLED
void OLED_WriteString(const char* str) {
    while (*str) {
        OLED_WriteChar(*str++);
    }
}

void USART1_Init() {
    RCC -> APB2ENR |= (1 << 4);  										// USART1 Clock
    RCC -> AHB1ENR |= (1 << 0);  										// GPIOA Clock

    GPIOA -> MODER &= ~((3 << (2 * 9)) | (3 << (2 * 10)));				// PA9 & 10 to Alternate Function
    GPIOA -> MODER |= (2 << (2 * 9)) | (2 << (2 * 10));

    GPIOA -> AFR[1] &= ~((0xF << (1 * 4)) | (0xF << (2 * 4)));			// Set to AFR7
    GPIOA -> AFR[1] |= (7 << (1 * 4)) | (7 << (2 * 4));

    USART1 -> BRR = 16000000 / 115200;  								// Baud rate 115200

    USART1 -> CR1 = (1 << 13) | (1 << 3)  | (1 << 2);					// Enabled transmitter, receiver and USART1
}

// Sends an AT command over USART1 to the ESP32
void Wifi_Send_Command(const char* str) {
	while (*str) {
		while (!(USART1->SR & (1<<7)));
		USART1 -> DR = *str;
		str++;
	}
}

// Retrieves a character from USART1 data register
char Wifi_Get_Char() {
    uint32_t timeout = 10000;
    while(!(USART1->SR & (1 << 5)) && timeout) {
        timeout--;
    }

    if(timeout == 0) return 0;

    return USART1->DR;
}

// Stores characters in a buffer which is parsed and then only contains the coin price.
// More error checking will be added to stop/restart if errors or non-ok responses are given.
void Wifi_Read_Response_With_Timeout(uint32_t timeout) {
    char c;
    uint8_t index = 0;
    uint8_t no_data_count = 0;

    memset(buffer, 0, MAX_BUFFER);

    for(volatile int i = 0; i < timeout; i++);

    while(no_data_count < 50) {
        c = Wifi_Get_Char();
        if(c != 0) {
            buffer[index] = c;
            if(index >= MAX_BUFFER - 2) break;
            index++;
            no_data_count = 0;
        } else {
            no_data_count++;
            for(volatile int i = 0; i < 1000; i++);
        }
    }
    buffer[index] = '\0';

    char *price_start = strstr(buffer, "\"USD\":");
    if(price_start) {
        char price_str[32];
        price_start += 6;
        int i = 0;
        while(price_start[i] != '}' && i < 31) {
            price_str[i] = price_start[i];
            i++;
        }
        price_str[i] = '\0';

        sprintf(buffer, "BTC: $%s", price_str);
        USART2_Print("Price for OLED: %s\r\n", buffer);
    } else {
        sprintf(buffer, "Error: Cannot retrieve price");
    }
}

// Will be changed to accept parameters but currently works as stands. Responses are sent to the buffer.
// Needed two CIPSTART calls for SSL connections to properly work.
void Wifi_HTTPS_GET() {
    USART2_Print("Starting SSL Connection\r\n");

    Wifi_Send_Command("ATE0\r\n");
    Wifi_Read_Response_With_Timeout(1000);

    Wifi_Send_Command("AT+CIPMUX=1\r\n");
    Wifi_Read_Response_With_Timeout(1000);

    Wifi_Send_Command("AT+CIPSTART=0,\"SSL\",\"min-api.cryptocompare.com\",443,2\r\n");
    Wifi_Read_Response_With_Timeout(7500);

    Wifi_Send_Command("AT+CIPSTART=0,\"SSL\",\"min-api.cryptocompare.com\",443,2\r\n");
    Wifi_Read_Response_With_Timeout(7500);

    const char *getRequest = "GET /data/price?fsym=BTC&tsyms=USD HTTP/1.1\r\n"
                            "Host: min-api.cryptocompare.com\r\n"
                            "Connection: close\r\n"
                            "User-Agent: ESP32/1.0\r\n"
                            "\r\n";

    USART2_Print("GET request length: %d\r\n", strlen(getRequest));
    char sendCmd[32];
    sprintf(sendCmd, "AT+CIPSEND=0,%d\r\n", strlen(getRequest));

    Wifi_Send_Command(sendCmd);
    Wifi_Read_Response_With_Timeout(3000);

    USART2_Print("Sending GET request...\r\n");
    Wifi_Send_Command(getRequest);
    Wifi_Read_Response_With_Timeout(12000);

    USART2_Print("\r\nRequest completed\r\n");
}

// 30 Second Interval Interrupt
void Timer2_Init() {
	RCC -> APB1ENR |= (1 << 0);
	RCC -> APB2ENR |= (1 << 14);

	TIM2 -> PSC |= 15999;
	TIM2 -> ARR = 1999;
	TIM2 -> DIER |= (1 << 0);

	NVIC -> ISER[0] |= (1 << 28);

	TIM2 -> CR1 |= (1 << 0);
}

void TIM2_IRQHandler() {
	if (TIM2 -> SR & (1 << 0)) {
		USART2_Print("Timer Interrupt....\r\n");
		OLED_SetCursor(0, 0);
		Wifi_HTTPS_GET();
		OLED_WriteString(buffer);
		OLED_SetCursor(30, 3);
		OLED_WriteString("Looks Good!");
		TIM2 -> ARR = 29999;
	}

	TIM2 -> SR &= ~(1 << 0);
}

//void find_addr() {
//    USART2_Print("Starting I2C scan...\r\n");
//
//    for (int i = 0; i < 128; i++) {
//        I2C_Start();
//        USART2_Print("Trying address: 0x%X\r\n", i);
//
//        I2C_SendAddress(i, 0);
//
//        if (!(I2C1->SR1 & (1 << 10))) {
//            USART2_Print("Found device at: 0x%X\r\n", i);
//        }
//
//        I2C1->SR1 &= ~(1 << 10);
//        I2C_Stop();
//    }
//
//    USART2_Print("Scan complete\r\n");
//}

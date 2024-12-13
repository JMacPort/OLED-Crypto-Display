#include "main.h"
#include <stdio.h>
#include <stdarg.h>

// Prototypes
void USART_Init();
void USART_Print(const char* str, ...);
void I2C_Init();
void OLED_Init();
//void find_addr();

// OLED
#define OLED_ADDR					0x3C
#define OLED_CMD					0x00

int main() {
	USART_Init();
	I2C_Init();
	OLED_Init();

	for (volatile int i = 0; i < 60000; i++);

	while (1) {

	}
}

void USART_Init() {
	RCC -> APB1ENR |= (1 << 17);										// USART2 Clock
	RCC -> AHB1ENR |= (1 << 0);											// GPIOA Clock

	GPIOA -> MODER &= ~((3 << (2 * 2)) | (3 << (2 * 3)));				// Sets mode to Alternate Function
	GPIOA -> MODER |= (2 << (2 * 2)) | (2 << (2 * 3));

	GPIOA -> AFR[0] |= (7 << (4 * 2)) | (7 << (4 * 3));					// AFR set to USART2

	USART2 -> BRR = 0x0683;												// Baud Rate 9600
	USART2 -> CR1 |= (1 << 3) | (1 << 13);								// Transmit Enable; USART2 Enable
}

void USART_Print(const char* str, ...) {
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
	while(!(I2C1->SR1 & (1 << 1)) && timeout) timeout--;  // Wait for ADDR with timeout
	if(timeout == 0) {
	   USART_Print("Address timeout\r\n");
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
    I2C_SendData(OLED_CMD);
    I2C_SendData(cmd);
    I2C_Stop();
}

void OLED_Init() {
    // Turn display off first
    OLED_Send_Command(0xAE);    // Display off

    // Power settings
    OLED_Send_Command(0x8D);    // Charge pump setting
    OLED_Send_Command(0x14);    // Enable charge pump

    // Turn display on
    OLED_Send_Command(0xAF);    // Display on
}

//void find_addr() {
//    USART_Print("Starting I2C scan...\r\n");
//
//    for (int i = 0; i < 128; i++) {
//        I2C_Start();
//        USART_Print("Trying address: 0x%X\r\n", i);
//
//        I2C_SendAddress(i, 0);
//
//        if (!(I2C1->SR1 & (1 << 10))) {
//            USART_Print("Found device at: 0x%X\r\n", i);
//        }
//
//        I2C1->SR1 &= ~(1 << 10);
//        I2C_Stop();
//    }
//
//    USART_Print("Scan complete\r\n");
//}











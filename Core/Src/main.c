#include "main.h"
#include <stdio.h>
#include <stdarg.h>
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
//void find_addr();

// OLED
#define OLED_ADDR					0x3C

char buffer[100];

int main() {
	USART1_Init();
	USART2_Init();
	I2C_Init();
	for (volatile int i = 0; i < 100000; i++);
	OLED_Init();

	for (volatile int i = 0; i < 100000; i++);
	OLED_Clear();
	OLED_SetCursor(45, 0);
	OLED_WriteString("BITCOIN");
	OLED_SetCursor(30, 3);
	OLED_WriteString("USD: $95,000");
	OLED_SetCursor(49, 6);
	OLED_WriteString("-6.3%");
//
//
//	for (volatile int i = 0; i < 10000; i++);
//	ESP_Send_Command("AT+UART_DEF=115200,8,1,0,0\r\n");
//	ESP_Response(buffer);
//
//	USART2_Print(buffer);
	while (1) {

	}
}

void USART2_Init() {
	RCC -> APB1ENR |= (1 << 17);										// USART2 Clock
	RCC -> AHB1ENR |= (1 << 0);											// GPIOA Clock

	GPIOA -> MODER &= ~((3 << (2 * 2)) | (3 << (2 * 3)));				// Sets mode to Alternate Function
	GPIOA -> MODER |= (2 << (2 * 2)) | (2 << (2 * 3));

	GPIOA -> AFR[0] |= (7 << (4 * 2)) | (7 << (4 * 3));					// AFR set to USART2

	USART2 -> BRR = 0x683;												// Baud Rate 9600
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
	while(!(I2C1->SR1 & (1 << 1)) && timeout) timeout--;  // Wait for ADDR with timeout
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

    // Clear all pixels
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
}

void OLED_SetCursor(uint8_t col, uint8_t page) {
    OLED_Send_Command(0x21);
    OLED_Send_Command(col);
    OLED_Send_Command(127);

    OLED_Send_Command(0x22);
    OLED_Send_Command(page);
    OLED_Send_Command(7);
}


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

void OLED_WriteString(const char* str) {
    while (*str) {
        OLED_WriteChar(*str++);
    }
}

void USART1_Init() {
    RCC -> APB2ENR |= (1 << 4);  									// USART1 Clock
    RCC -> AHB1ENR |= (1 << 0);  									// GPIOA Clock

    GPIOA -> MODER &= ~((3 << (2 * 9)) | (3 << (2 * 10)));			// PA9 & 10 to Alternate Function
    GPIOA -> MODER |= (2 << (2 * 9)) | (2 << (2 * 10));

    GPIOA -> AFR[1] &= ~((0xF << (1 * 4)) | (0xF << (2 * 4)));		// Set to AFR7
    GPIOA -> AFR[1] |= (7 << (1 * 4)) | (7 << (2 * 4));

    USART1->BRR = 0x460;  											// Baud rate 115200

    USART1->CR1 = (1 << 13) | (1 << 3)  | (1 << 2);					// Enabled transmitter, receiver and USART1


}

// Sends a command from a string to the ESP-01 module
void ESP_Send_Command(const char* cmd) {
    USART2_Print("Sending: ");
    while (*cmd) {
        USART2_Print("0x%02X ", (unsigned char)*cmd);
        while (!(USART1->SR & (1 << 7)));
        USART1->DR = *cmd;
        cmd++;
    }
    USART2_Print("\r\n");
}

// Gets the data register and returns a character
char ESP_Get_Char() {
	if(USART1 -> SR & (1 << 5)) {
        char data = USART1->DR;
        USART2_Print("Got byte: 0x%02X\r\n", (unsigned char)data);
        return data;
    }

    USART2_Print("No data\r\n");
    return 0;
}

// Strings together the response and stores in the buffer
void ESP_Response(char* buffer) {
    int i = 0;

    for(int count = 0; count < 100; count++) {
        buffer[i] = ESP_Get_Char();
        if(buffer[i] == '\n') break;  // Exit on newline
        i++;
    }
    buffer[i] = '\0';
}

/*
 * Still very many bugs I need to deal with but I have ordered an external power breakout board so I
 * no longer need to rely on the MCU to drive power. I think this was the same issue I had with the
 * micro SD card modules. It arrives in a few days so I will be able to test better then, the output
 * I was receiving from the ESP-01 was inconsistent at best and seemed to be unstable. I measured the
 * voltage of the TX pin on the ESP when it was sending responses and it was either too low or fluctuated
 * quite a bit. Hoping I can fix it with a more stable power source.
 *
 * Also, will convert the USART functionality to DMA when everything is confirmed working for better
 * optimization but I would rather focus first on getting the modules to work.
 */







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

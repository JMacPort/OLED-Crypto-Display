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
void Wifi_Init();
//void find_addr();

// OLED
#define OLED_ADDR					0x3C

#define MAX_BUFFER					1000

char buffer[MAX_BUFFER];

int main() {
	USART1_Init();
	USART2_Init();
	I2C_Init();

//	// Change to timer based so setup time is constant
//	for (volatile int i = 0; i < 100000; i++);
	USART2_Print("Ready....\r\n");
	Wifi_Init();
//	OLED_Init();
//
//	for (volatile int i = 0; i < 100000; i++);
//	// Move to receive interrupt
//	OLED_Clear();
//	OLED_SetCursor(45, 0);
//	OLED_WriteString("BITCOIN");
//	OLED_SetCursor(30, 3);
//	OLED_WriteString("USD: $95,000");
//	OLED_SetCursor(49, 6);
//	OLED_WriteString("-6.3%");


	while (1) {
	}
}

// From here ->
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
// -> Here is fine and does not need to be


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

// Should be fine, not doing any extensive work so not worried about this
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

// Fine as well
void OLED_SetCursor(uint8_t col, uint8_t page) {
    OLED_Send_Command(0x21);
    OLED_Send_Command(col);
    OLED_Send_Command(127);

    OLED_Send_Command(0x22);
    OLED_Send_Command(page);
    OLED_Send_Command(7);
}

// Displays one character from the bitmap, should be fine
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

// Displays a string on OLED, should also be fine
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

// Stores characters in a buffer. Timeout is gives a delay to start the function and data count is used to make sure data is being retrieved
void Wifi_Read_Response_With_Timeout(uint32_t timeout) {
    char c;
    uint8_t index = 0;
    uint8_t no_data_count = 0;

    memset(buffer, 0, MAX_BUFFER);

    for(volatile int i = 0; i < timeout; i++);

    while(no_data_count < 50) {
        c = Wifi_Get_Char();
        if(c != 0) {
            USART2_Print("%c", c);

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
}

// I have to figure out the issues with SSL but currently this connects to httpbin and gets a response with a GET request
void Wifi_Init() {
    USART2_Print("Starting HTTP Test\r\n");


    Wifi_Send_Command("AT+CIPMUX=1\r\n");
    Wifi_Read_Response_With_Timeout(1000);


    USART2_Print("Connecting to server...\r\n");
    Wifi_Send_Command("AT+CIPSTART=0,\"TCP\",\"httpbin.org\",80\r\n");
    Wifi_Read_Response_With_Timeout(10000);

    const char *httpRequest = "GET /get HTTP/1.0\r\n"
                             "Host: httpbin.org\r\n"
                             "Connection: close\r\n\r\n";

    char sendCmd[32];
    sprintf(sendCmd, "AT+CIPSEND=0,%d\r\n", strlen(httpRequest));

    Wifi_Send_Command(sendCmd);
    Wifi_Read_Response_With_Timeout(1000);

    USART2_Print("Sending request...\r\n");
    Wifi_Send_Command(httpRequest);

    USART2_Print("\r\nResponse:\r\n");
    Wifi_Read_Response_With_Timeout(10000);
    USART2_Print("%s\r\n", buffer);
}

/*
 * Got an ESP32 to work in AT mode so that is what I will use for the wifi connectivity of this project.
 */


/*
 * Next Steps:
 * - Write/Read functionality for USART1/Bluetooth
 * - Connect Bluetooth module to open COM port.
 * - Create interrupts to recieve and send data to py script
 * - Data sent will be to refresh current price, change coin or swap screen output
 * - Data received will be used to write to OLED in predefined spots. ie. col = 5, page = 10;
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

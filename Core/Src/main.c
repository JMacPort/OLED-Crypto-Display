#include "main.h"

int main() {

}

void USART_Init() {
	RCC -> APB1ENR |= (1 << 17);										// USART2 Clock
	RCC -> AHB1ENR |= (1 << 0);											// GPIOA Clock

	GPIOA -> MODER &= ~((3 << (2 * 2)) | (3 << (2 * 3)));				// Sets mode to Alternate Function
	GPIOA -> MODER |= (2 << (2 * 2)) | (2 << (2 * 3));

	GPIOA -> AFR[0] |= (7 << (4 * 2)) | (7 << (4 * 3));					// AFR set to USART2

	USART2 -> BRR = 0x0683;												// Baud Rate 9600
	USART2 -> CR1 |= (1 << 3) | (1 << 13);											// Transmit Enable; USART2 Enable

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

	while (!(I2C1 -> SR1 & (1 << 1)));

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

void find_addr() {



}











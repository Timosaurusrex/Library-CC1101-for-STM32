/**
  ******************************************************************************
  * @file    cc1101.c 
  * @author  
  * @version V1.0.0
  * @date    29-05-2024
  * @brief   Contains all functions for interfacing with CC1101
  ******************************************************************************
**/

#include "funkmodul_header.h"
#include <stdint.h>
#include <stdio.h>

void init_port(void) {
    int temp;

    RCC->APB2ENR |= 0x4; // enable clock for GPIOA
     
    temp = GPIOA->CRL;  // Save Configuration Ports PA0-PA7 in temp Variable  
		temp &= 0x0000FF00; 
    temp |= 0x38330088;
    GPIOA->CRL = temp;  // Update new Configuration 
} 
		
void uart3_init(void) {
  RCC->APB2ENR |= 0x8;          // GPIOB Clock Enable

  GPIOB->CRH &= 0xFFFFF0FF;     // reset  PB10 configuration-bits 
  GPIOB->CRH |= 0xB00;          // Tx (PB10) - alt. out push-pull

  RCC->APB1ENR |= 0x40000;      // USART3 mit einem Takt versrogen

  USART3->CR1 &= ~0x1000;       // M: Word length:0 --> Start bit, 8 Data bits, n Stop bit
  USART3->CR1 &= ~0x0400;       // PCE (Parity control enable):0 --> No Parity

  USART3->CR2 &= ~0x3000;       // STOP:00 --> 1 Stop bit

  USART3->BRR  = 0xEA6;         // set Baudrate to 9600 Baud (SysClk 36Mhz)

  USART3->CR1 |= 0x0C;          // enable  Receiver and Transmitter
  USART3->CR1 |= 0x2000;        // Set USART Enable Bit
}

void TIM3_Config(void)
{
	/*-------------------------------------- Timer 3 konfigurieren ------------------------------------*/
	RCC->APB1ENR |= 0x0002;	    // TIM3 Clock enable
	TIM3->SMCR = 0x0000;        // Slave Mode disabled - CK_INT wird verwendet
	TIM3->CR1 = 0x0000;	    // Upcounter: Zählt von 0 bis zum Wert des Autoreload-Registers
        TIM3->CR1 |= 0x0080;        // ARPE-Bit setzen (Auto Reload Register buffered)
	TIM3->PSC = 0x3B;	    // Prescaler - dez. 59
	TIM3->ARR = 0xEA60;         // Auto-Reload Register - dez. 60000
	TIM3->DIER = 0x01;          // Enable Update Interrupt
	NVIC_init(TIM3_IRQn,0);     // Enable Interrupt Timer3 at NVIC (Priority 0)
	TIM3->CR1 |= 0x0001;				//starte counter
}

void ClockConfig(void)
{
    FLASH->ACR = 0x12;       // Flash-Latenz auf 2 Wartezyklen einstellen
    RCC->CR |= RCC_CR_HSEON; // HSE einschalten
    RCC->CFGR |= RCC_CFGR_PLLXTPRE_HSE_Div2;
    while ((RCC->CR & RCC_CR_HSERDY) == 0); // Warten, bis HSE bereit ist
    RCC->CFGR &= ~RCC_CFGR_PLLMULL; // PLLMULL-Bits löschen
    RCC->CFGR |= RCC_CFGR_PLLMULL9; // 4 * 9 = 36 MHz (SYSCLK)
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // ADCCLK = SYSCLK/6 (APB2 PRESCALER=1)
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;  // PCLK1 (APB1) = SYSCLK/2 (HCLK=SYSCLK)
    RCC->CFGR |= RCC_CFGR_PLLSRC;      // PLL-Quelle = HSE
    RCC->CR |= RCC_CR_PLLON; // PLL einschalten
    while ((RCC->CR & RCC_CR_PLLRDY) == 0); // Warten, bis PLL bereit ist
    RCC->CFGR |= RCC_CFGR_SW_PLL; // PLL = Systemtakt
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL) == 0);
    // Warten, bis der Systemtakt stabil ist
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    // Ende der Takteinstellung
}

void NVIC_init(char position, char priority) 
{	
	//Interrupt priority register: Setzen der Interrupt Priorität
	NVIC->IP[position]=(priority<<4);	
	//Interrupt Clear Pendig Register: Verhindert, dass der Interrupt auslöst sobald er enabled wird 
	NVIC->ICPR[position >> 0x05] |= (0x01 << (position & 0x1F));
	//Interrupt Set Enable Register: Enable interrupt
	NVIC->ISER[position >> 0x05] |= (0x01 << (position & 0x1F));
}

void uart3_put_char(char characters) {
  while (!(USART3->SR & 0x80));		// wait, until last datas have been sent
  USART3->DR = characters;      	// write datas in send-register
}

void uart3_put_string(char *string) {
  while (*string) {
      uart3_put_char(*string++);
  }
}

void CC1101_init(void) {
	int i;
	uint8_t PaTabel[8] = {0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60};
	
	CSN_PIN = 1;
	SCK_PIN = 1;
	MOSI_PIN = 0;
	
	// Wait for CC1101 to initialize
	for (i = 0; i < 10000; i++);
	
	reset_CC1101();		
	RegConfigSettings(F_433);		         	//CC1101 register config
	SpiWriteBurstReg(CC1101_PATABLE,PaTabel,8);		//CC1101 PATABLE config
}

void reset_CC1101(void){
	 CSN_PIN = 0;
	 waitloop_10us(20);
	 CSN_PIN = 1;
	 waitloop_10us(20);
	 CSN_PIN = 0;
	 while(MISO_PIN == 1);
	 SPI1_Transfer(CC1101_SRES);
	 while(MISO_PIN == 1);
	 CSN_PIN = 1;
}

void waitloop_10us(int us) {
	int j = 0;
  	for (j = 0; j < 27 * us; j++);
}

uint8_t SpiReadStatus(uint8_t addr){
	uint8_t value,temp;
	temp = addr | READ_BURST;
	CSN_PIN = 0;
	while(MISO_PIN == 1);
	SPI1_Transfer(temp);
	value = SPI1_Transfer(0x00);
	CSN_PIN = 1;
	
	return value;
}

void SpiWriteReg(uint8_t addr, uint8_t value)
{
	CSN_PIN = 0;
	while(MISO_PIN == 1);
	SPI1_Transfer(addr);
	SPI1_Transfer(value);
	CSN_PIN = 1;
}

void SpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num)
{
	uint8_t i, temp;
	
	temp = addr | WRITE_BURST;
	CSN_PIN = 0;
	while(MISO_PIN == 1);
	SPI1_Transfer(temp);
	for (i = 0; i < num; i++)
	{
		SPI1_Transfer(buffer[i]);
	}
	CSN_PIN = 1;
}

void SpiStrobe(uint8_t strobe)
{
	CSN_PIN = 0;
	while(MISO_PIN == 1);
	SPI1_Transfer(strobe);
	CSN_PIN = 1;
}

uint8_t SpiReadReg(uint8_t addr) 
{
	uint8_t temp, value;

  	temp = addr|READ_SINGLE;
	CSN_PIN = 0;
	while(MISO_PIN == 1);
	SPI1_Transfer(temp);
	value=SPI1_Transfer(0);
	CSN_PIN = 1;

	return value;
}

void SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num)
{
	uint8_t i,temp;

	temp = addr | READ_BURST;
	CSN_PIN = 0;
	while(MISO_PIN == 1);
	SPI1_Transfer(temp);
	for(i=0;i<num;i++)
	{
		buffer[i]=SPI1_Transfer(0);
	}
	CSN_PIN = 1;
}

uint8_t SPI1_Transfer(uint8_t data) {
	int i;
	uint8_t buffer;
	
	SCK_PIN = 0;
	for (i = 7; i >= 0; --i) {
	waitloop_10us(2);
	MOSI_PIN = ((data >> i) & 0x01);
	buffer = (buffer << 1) | MISO_PIN;
	waitloop_10us(3);
	SCK_PIN = 1;
	waitloop_10us(6);
	SCK_PIN = 0;
	}
		return buffer;
}


void SendData(uint8_t *txBuffer,uint8_t size)
{
	SpiWriteReg(CC1101_TXFIFO,size);
	SpiWriteBurstReg(CC1101_TXFIFO,txBuffer,size);			//write data to send
	SpiStrobe(CC1101_STX);						//start send	
	while (GDO0 != 1);						// Wait for GDO0 to be set -> sync transmitted  
	while (GDO0 == 1);						// Wait for GDO0 to be cleared -> end of packet
	SpiStrobe(CC1101_SFTX);						//flush TXfifo
}

void SetReceive(void)
{
	SpiStrobe(CC1101_SRX);
}

uint8_t ReceiveData(uint8_t *rxBuffer)
{
	uint8_t size;
	uint8_t status[2];

	if(SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO)
	{
		size=SpiReadReg(CC1101_RXFIFO);
		SpiReadBurstReg(CC1101_RXFIFO,rxBuffer,size);
		SpiReadBurstReg(CC1101_RXFIFO,status,2);
		SpiStrobe(CC1101_SFRX);
		return size;
	}
	else
	{
		SpiStrobe(CC1101_SFRX);
		return 0;
	}
}

uint8_t CheckReceiveFlag(void)
{
	char ausgabe[50];
	
	if(GDO0==1)			//receive data
	{
		while (GDO0==1);
		sprintf(ausgabe,"status.txt=\"data_rec\"\xff\xff\xff");
		uart3_put_string(&ausgabe[0]); 
		return 1;
	}
	else				// no data
	{
		sprintf(ausgabe,"status.txt=\"data_not\"\xff\xff\xff");
		uart3_put_string(&ausgabe[0]); 
		return 0;
	}
}

void RegConfigSettings(char f) 
{
    SpiWriteReg(CC1101_FSCTRL1,  0x08);
    SpiWriteReg(CC1101_FSCTRL0,  0x00);
	
    switch(f)
    {
      case F_868:
      	SpiWriteReg(CC1101_FREQ2,    F2_868);
      	SpiWriteReg(CC1101_FREQ1,    F1_868);
      	SpiWriteReg(CC1101_FREQ0,    F0_868);
        break;
      case F_915:
        SpiWriteReg(CC1101_FREQ2,    F2_915);
        SpiWriteReg(CC1101_FREQ1,    F1_915);
        SpiWriteReg(CC1101_FREQ0,    F0_915);
        break;
	  case F_433:
        SpiWriteReg(CC1101_FREQ2,    F2_433);
        SpiWriteReg(CC1101_FREQ1,    F1_433);
        SpiWriteReg(CC1101_FREQ0,    F0_433);
        break;
	  default: // F must be set
	  	break;
	}
	
    SpiWriteReg(CC1101_MDMCFG4,  0x5B);
    SpiWriteReg(CC1101_MDMCFG3,  0xF8);
    SpiWriteReg(CC1101_MDMCFG2,  0x03);
    SpiWriteReg(CC1101_MDMCFG1,  0x22);
    SpiWriteReg(CC1101_MDMCFG0,  0xF8);
    SpiWriteReg(CC1101_CHANNR,   0x00);
    SpiWriteReg(CC1101_DEVIATN,  0x47);
    SpiWriteReg(CC1101_FREND1,   0xB6);
    SpiWriteReg(CC1101_FREND0,   0x10);
    SpiWriteReg(CC1101_MCSM0 ,   0x18);
    SpiWriteReg(CC1101_FOCCFG,   0x1D);
    SpiWriteReg(CC1101_BSCFG,    0x1C);
    SpiWriteReg(CC1101_AGCCTRL2, 0xC7);
    SpiWriteReg(CC1101_AGCCTRL1, 0x00);
    SpiWriteReg(CC1101_AGCCTRL0, 0xB2);
    SpiWriteReg(CC1101_FSCAL3,   0xEA);
    SpiWriteReg(CC1101_FSCAL2,   0x2A);
    SpiWriteReg(CC1101_FSCAL1,   0x00);
    SpiWriteReg(CC1101_FSCAL0,   0x11);
    SpiWriteReg(CC1101_FSTEST,   0x59);
    SpiWriteReg(CC1101_TEST2,    0x81);
    SpiWriteReg(CC1101_TEST1,    0x35);
    SpiWriteReg(CC1101_TEST0,    0x09);
    SpiWriteReg(CC1101_IOCFG2,   0x0B); 	//serial clock.synchronous to the data in synchronous serial mode
    SpiWriteReg(CC1101_IOCFG0,   0x06);  	//asserts when sync word has been sent/received, and de-asserts at the end of the packet 
    SpiWriteReg(CC1101_PKTCTRL1, 0x04);		//two status bytes will be appended to the payload of the packet,including RSSI LQI and CRC OK
    SpiWriteReg(CC1101_PKTCTRL0, 0x05);		//whitening off;CRC Enable£»variable length packets, packet length configured by the first byte after sync word
    SpiWriteReg(CC1101_ADDR,     0x00);		//address used for packet filtration.
    SpiWriteReg(CC1101_PKTLEN,   0x3D); 	//61 bytes max length
}

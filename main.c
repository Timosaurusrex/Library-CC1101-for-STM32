/**
 * @file main.c
 * @brief Hauptprogramm für die Kommunikation mit einem CC1101 Funkmodul und einem Nextion Display
 *
 * Dieses Programm initialisiert ein STM32F10x-Mikrocontroller-System, um Daten zu senden und zu empfangen
 * über ein CC1101 Funkmodul. Zusätzlich wird ein Nextion Display verwendet, um Benutzereingaben zu verarbeiten
 * und Statusinformationen anzuzeigen. Das Programm bietet sowohl Sende- als auch Empfangsfunktionen und nutzt
 * Timer-Interrupts zur Zeiterfassung.
 *
 * Funktionen:
 * - Initialisierung der Hardware (Ports, UART, CC1101, Timer)
 * - Verarbeitung von Touch-Events auf dem Nextion Display
 * - Senden und Empfangen von Datenpaketen über das CC1101 Funkmodul
 * - Anzeige von Statusinformationen auf dem Nextion Display
 *
 * @version V1.0.0
  * @date    29-05-2024
 */

#include <stm32f10x.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <Nextion.h>
#include "funkmodul_header.h"

void TIM3_IRQHandler(void);
void ausgabeDisplay_transmit(void);
void ausgabeDisplay_receive(uint8_t data[]);
void touch_event(NexEventObj*ptr,char out[], size_t size);

int button = 3, zsek = 0, sek = 0, min = 0, std = 0;

NexEventObj button1;
NexEventObj button2;
NexEventObj button3;
NexEventObj button4;

int main(void) {
		char ausgabe[50];
		
		uint8_t PaTabel[8] = {0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60};
		
		init_port();
		uart3_init();		
		CC1101_init();
		
		ClockConfig();
		TIM3_Config();
		
		int tran_rec = 0;
		
		if(tran_rec == 1){
			/*Transmit data*/
			NexEventObj *nextlisten_list[] = {&button1, &button2, &button3, &button4, NULL}; // NULL terminierte Liste von allen Touch events
			
			button1.pid = 0;      						//Hauptseite ID
			button1.cid = 8;      						//Komponenten ID  of "Schalter Button"
			button1.name= "b0";   						//Einzigartiger Name auf der Hauptseite
			button1.pushhandler=touch_event; //Callback Funktion bei loslassen des Tasters

			button2.pid = 0;      						//Hauptseite ID
			button2.cid = 9;      						//Komponenten ID  of "Schalter Button 2"
			button2.name= "b1";   						//Einzigartiger Name auf der Hauptseite
			button2.pushhandler=touch_event; //Callback Funktion bei loslassen des Tasters

			button3.pid = 0;      						//Hauptseite ID
			button3.cid = 0x0A;      						//Komponenten ID  of "Schalter Button 3"
			button3.name= "b2";   						//Einzigartiger Name auf der Hauptseite
			button3.pushhandler=touch_event; //Callback Funktion bei loslassen des Tasters

			button4.pid = 0;      						//Hauptseite ID
			button3.cid = 0x0B;      						//Komponenten ID  of "Schalter Button 4"
			button3.name= "b3";   						//Einzigartiger Name auf der Hauptseite
			button3.pushhandler=touch_event; //Callback Funktion bei loslassen des Tasters
			
			Nex_Init(0);
			
			//txData[6] = {button, zsek, sek, min, std, 0x00};
			uint8_t txData[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
			
			while(1){
					sprintf(ausgabe,"sta.txt=\"Transmit\"\xff\xff\xff");
					uart3_put_string(&ausgabe[0]);
				
					Nex_Event_Loop(nextlisten_list);
				
					txData[0] = button;
					txData[1] = zsek;
					txData[2] = sek;
					txData[3] = min;
					txData[4] = std;
				
					SendData(txData, sizeof(txData));
					ausgabeDisplay_transmit(); 
			}
		}else{
			/*Receive data*/
			uint8_t rxData[6] = {0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};
				
			while(1){
				sprintf(ausgabe,"sta.txt=\"Receive\"\xff\xff\xff");
				uart3_put_string(&ausgabe[0]);
				
				SetReceive();
				CheckReceiveFlag();
				ReceiveData(rxData);
				
				ausgabeDisplay_receive(rxData); 	
				CC1101_init();
			}
		}
}

void TIM3_IRQHandler(void)
{	
	TIM3->SR &=~0x00FF;
	zsek++;
	if(zsek>=10){
			sek++;
			zsek=0;
	}
	if(sek>=60){
			min++;
			sek=0;
	}
	if(min>=60){
			std++;
			min=0;}
	if(std>=24){
			std=0;
	}
	return;
}

void ausgabeDisplay_transmit(void)
{
		char ausgabe[50];
		sprintf(ausgabe,"data.txt=%d\xff\xff\xff",button);
    uart3_put_string(&ausgabe[0]); 
    sprintf(ausgabe,"Stunden.val=%d\xff\xff\xff",std);
    uart3_put_string(&ausgabe[0]); 
    sprintf(ausgabe,"Minuten.val=%d\xff\xff\xff",min);
    uart3_put_string(&ausgabe[0]); 
    sprintf(ausgabe,"Sekunden.val=%d\xff\xff\xff",sek);
    uart3_put_string(&ausgabe[0]); 
    sprintf(ausgabe,"Zehntelsek.val=%d\xff\xff\xff",zsek);
    uart3_put_string(&ausgabe[0]); 
}

void touch_event(NexEventObj*ptr,char out[], size_t size)
{
	uint16_t number;	
	uint16_t len;		
	char buffer[10];	
	
	memset(buffer, 0, sizeof(buffer));	//Buffer leeren
  /* Ruft den Textinhalt der Schaltflächenkomponente ab [Wert ist vom Typ string]*/
	NexBut_getText(ptr, buffer, sizeof(buffer));	
	 
	if(buffer[0] == '1'){
		button = 1;		
	}else if(buffer[0] == '2'){	
		button = 2;		
	} else if(buffer[0] == '3'){	
		button = 2;		
	} else if(buffer[0] == '4'){	
		button = 4;	
	}
}

void ausgabeDisplay_receive(uint8_t data[])
{
	char ausgabe[50];
	sprintf(ausgabe,"data.txt=%d\xff\xff\xff", data[0]);
	uart3_put_string(&ausgabe[0]); 
	sprintf(ausgabe,"Stunden.val=%d\xff\xff\xff", data[1]);
	uart3_put_string(&ausgabe[0]); 
	sprintf(ausgabe,"Minuten.val=%d\xff\xff\xff", data[2]);
	uart3_put_string(&ausgabe[0]); 
	sprintf(ausgabe,"Sekunden.val=%d\xff\xff\xff", data[3]);
	uart3_put_string(&ausgabe[0]); 
	sprintf(ausgabe,"Zehntelsek.val=%d\xff\xff\xff", data[4]);
	uart3_put_string(&ausgabe[0]); 
}



























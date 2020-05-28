// !!! check ^ manually after Clion auto format
#include <reg52.h>
#include "sht3x.h"
#include "typedefs.h"

#define SEND_BUF_SIZE 8
uint8 r_buf;
int temperature;
sbit bluetooth_state = P3^6;
sbit bluetooth_led = P2^3;
unsigned int total_times = 0; // used in Timer to periodically report data


typedef enum {
	NOT_CONNECTED = 0,
	CONNECTED_AND_RUNNING = 1,
	CONNECTED_AND_HALTED = 2,
} connectStatus;

connectStatus connection = NOT_CONNECTED;

void delay_sometime(uint16 t) {
    if (t <= 0) {
        return;
    } else {
        uint8 j = 0;
        for (j = 0; j < 200; j++) {
            while (t != 0) {
                t--;
            }
        }
    }
}
void init_serial(void) {
    SCON = 0x50; // mode 1, 8-bit UART
    PCON = 0x00;
    TMOD=0x21;

//    TMOD |= 0x20; // TMOD: timer 1, mode 2, 8-bit reload
    // baud rate: 4800Hz
    TH1 = 253; //0xfd
    TL1 = 253;
    TR1 = 1;

//    TMOD|=0x01;
    TH0=76;
    TL0=0;
    ET0=1;
    TR0=1;

    ES = 1;
    EA = 1;
}
void serial_send(uint8 c) {
    TI = 0;
    SBUF = c;
    while (!TI);
    TI = 0;
}


void send_temperature(int temperature) {
    // sned temperature as string
    uint8 send_buf[SEND_BUF_SIZE];
    uint8 i;
    send_buf[0] = (temperature / 10000) + '0';
    send_buf[1] = (temperature % 10000 / 1000) + '0';
    send_buf[2] = (temperature % 1000 / 100) + '0';
    send_buf[3] = (temperature % 100 / 10) + '0';
    send_buf[4] = '.';
    send_buf[5] = (temperature % 10) + '0';
    send_buf[6] = '\n';
    send_buf[7] = '\0';

    for (i = 0; i < SEND_BUF_SIZE; i++) {
				if (send_buf[i] != '\0') {
					serial_send(send_buf[i]);
				}
        
    }
}

void main() {
	init_sht3x();
	delay_sometime(50);
	init_serial();
	delay_sometime(50);
	while (1) {
        // everything is handled in interrupt fn
        // blank here
	}
}


void UARTInterrupt(void) interrupt 4 {
    if (RI) {
        RI = 0;

        r_buf = SBUF;
        if(r_buf == '1') {
					// reset
					init_sht3x();
					delay_sometime(50);
					init_serial();
					delay_sometime(50);
					connection = CONNECTED_AND_RUNNING;
        } else if(r_buf == '2') {
					// reset
					init_sht3x();
					delay_sometime(50);
					init_serial();
					delay_sometime(50);
					connection = CONNECTED_AND_HALTED;
        }
    }
}


// read data every 2s
void time0() interrupt 1 {
    EA=0;
    total_times += 1;
    if (total_times == 20) {
        total_times = 0;

        if (connection == CONNECTED_AND_RUNNING) {
        init_sht3x();
        delay_sometime(50);
        init_serial();
        delay_sometime(50);
        read_temperature(&temperature);
        send_temperature(temperature);
        }
    }
    TH0=0x4c;
    TL0=0x00;
    EA=1;
}


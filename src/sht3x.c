// forked from vendor sample code pdf: Sensirion_Humidity_Sensors_SHT3x_Sample_Code_V2.pdf :: sht3x.h

#include "sht3x.h"
#define POLYNOMIAL  0x31 // P(x) = x^8 + x^5 + x^4 + 1 = 00110001
// make PCB with pull up resistors on P2
sbit SDA = P2^1;
sbit SCL = P2^0;

/*SHT3X_Init(0x45); // Address: 0x44 = Sensor on EvalBoard connector
    0x45 = Sensor on EvalBoard
     */
#define _i2cAddress  0x44

void delay_cycle(uint16 t) {
    if (t <= 0) {
        return;
    } else {
        uint8 j = 0;
        for (j=0;j<100;j++){
            while (t != 0) {
                t--;
            }
        }
    }
}
uint8 integrity_check(uint8 *checksum_data, uint8 bytes_number) {
    uint8 a_bit;
    uint8 checksum = 0xFF;
    uint8 i;

    for (i = 0; i < bytes_number; i++) {
        checksum ^= (checksum_data[i]);
        for (a_bit = 8; a_bit > 0; --a_bit) {
            if (checksum & 0x80) {
                checksum = (checksum << 1) ^ POLYNOMIAL;
            }
            else {
                checksum = (checksum << 1);
            }
        }
    }
    return checksum;
}

void i2c_start_condition(void) {
    // !! originally, all delays use: while(t--), not delay_cycle as that in reference pdf
    SDA = 1;
    delay_cycle(1);
    SCL = 1;
    delay_cycle(1);
    SDA = 0;
    delay_cycle(10);
    SCL = 0;
    delay_cycle(10);
}

void i2c_stop_condition(void) {
    SCL = 0;
    delay_cycle(1);
    SDA = 0;
    delay_cycle(1);
    SCL = 1;
    delay_cycle(10);
    SDA = 1;
    delay_cycle(10);
}

etError send_one_byte(uint8 to_write) {
    etError error = NO_ERROR;
    uint8 mask;
    SCL = 0;

    for (mask = 0x80; mask > 0; mask >>= 1) {
        if ((mask & to_write) == 0) {
            SDA = 0;
        } else {
            SDA = 1;
        }
        delay_cycle(1); //originally, 10
        SCL = 1;
        delay_cycle(5); //50
        SCL = 0;
        delay_cycle(1); //10
    }
    SDA = 1;
    SCL = 1;
    delay_cycle(1); // 20
    if (SDA == 1) {
        error = ACK_ERROR;
    }
    SCL = 0;
    delay_cycle(20); // 50
    return error;
}

uint8 recv_one_byte(etI2cAck ack) {
    etError error = NO_ERROR;
    uint8 data_read = 0x00;
    uint8 mask;
    SCL = 0;

    SDA = 1; // release SDA-line
    for (mask = 0x80; mask > 0; mask >>= 1) {
        SCL = 1;
        delay_cycle(4);
        if (SDA == 1) {
            data_read |= mask;
        }
        SCL = 0;
        delay_cycle(1);
    }
    if (ack == ACK) {
        SDA = 0;
    } else {
        SDA = 1;
    }
    delay_cycle(1);
    SCL = 1;
    delay_cycle(5);
    SCL = 0;
    SDA = 1;
    delay_cycle(20);

    return data_read;
}

etError sht3x_write_command(etCommands command) {
    etError error; // error code
    // write the upper 8 bits of the command to the sensor
    error = send_one_byte(command >> 8);
    // write the lower 8 bits of the command to the sensor
    error |= send_one_byte(command & 0xFF);
    return error;
}

etError sht3x_start_write_access(void) {
    etError error; // error code
    // write a start condition
    i2c_start_condition();
    // write the sensor I2C address with the write flag
    error = send_one_byte(_i2cAddress << 1);
    return error;
}

etError sht3x_clear_all_alert_flags(void) {
    etError error; // error code
    error = sht3x_start_write_access();

    if (error == NO_ERROR) {
        // if no error, write clear status register command
        error = sht3x_write_command(CMD_CLEAR_STATUS);
    }
    i2c_stop_condition();
    return error;
}

void init_sht3x(void) {
    // I2C-bus idle mode SDA released
    SDA = 1;
    // I2C-bus idle mode SCL released
    SCL = 1;

    sht3x_start_write_access();
    sht3x_clear_all_alert_flags();
    
    sht3x_start_write_access();
    sht3x_write_command(CMD_MEAS_PERI_1_H); // measurement: periodic 1 mps, high repeatability
    i2c_stop_condition();
}

etError read_temperature(int *temperature) {
    etError error;
    unsigned long data_raw;
    uint8 data_array[6];
    i2c_start_condition();
    error = send_one_byte(_i2cAddress << 1);
    if (error == NO_ERROR) {
        error = sht3x_write_command(CMD_FETCH_DATA); // readout measurements for periodic mode
    }
    // wait until finished
    if (error == NO_ERROR) {
        i2c_start_condition();
        error = send_one_byte(_i2cAddress << 1 | 0x01);
    }
    // receive raw sensor data
    if (error == NO_ERROR) {
        uint8 i;
        for (i = 0; i < 5; i++) {
            data_array[i] = recv_one_byte(ACK);
        }
        data_array[i] = recv_one_byte(NACK);
        i2c_stop_condition();

        // perform data integrity check
        if (data_array[2] != integrity_check(data_array, 2)) {
            error = CHECKSUM_ERROR;
        }
        if (data_array[5] != integrity_check(&data_array[3], 2)) {
            error = CHECKSUM_ERROR;
        }
    }
    // calculate temperature from received data
    if (error == NO_ERROR) {
        data_raw = (data_array[0] << 8) | data_array[1];
        *temperature = (int)(1750 * data_raw / 65535 - 450);
    }
    return error;
}

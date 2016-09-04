#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

int input(int range) {
    int temp;
    time_t t;
    srand((unsigned) time(&t));
    temp = rand() % range;
    printf("\n temp: %i", temp);
    return (temp);
}

void convert(char* name) {
    double tempInput;
    double output;
    char outputprint[50];
    char tempoutput[15];

    /* SRF04 is an ultrasonic sensor.
     * The width of the pulse is measured in microseconds.
     * Dividing by 58 gives output distance in cm and dividing by 148 in inches.
     * Output range - maximum 3m
     */

    if (strcmp(name, "SRF04") == 0) {
        output = input(17461) / 58.2; //cm
        sprintf(outputprint, "%G", output);
    }/* SRF05 is an ultrasonic sensor.
         * The width of the pulse is measured in microseconds.
         * Dividing by 5.8 gives output distance in cm and dividing by 14.8 in inches.
         * Output range - maximum 3m
         */

    else if (strcmp(name, "SRF05") == 0) {
        output = input(1741) / 5.8; //cm
        sprintf(outputprint, "%G", output);
    }/*
         * LM35 is a temperature sensor
         * Range: -55 to 150 degree Celsius
         * Temperature = output voltage in mV / 10     
         */

    else if (strcmp(name, "LM35") == 0) {
        output = (input(2051) / 10) - 55; // Celsius
        sprintf(outputprint, "%G", output);
    }/*
         * TM35 is a temperature sensor
         * Range: 10 to 125 degree Celsius
         * Temperature = output voltage in mV / 10     
         */

    else if (strcmp(name, "LM35") == 0) {
        output = (input(1151) / 10) + 10; // Celsius
        sprintf(outputprint, "%G", output);
    }/*
         * SPC 1000 is a pressure  sensor
         * Range: 30 to 120 kPa
         * Output is the absolute value
         */

    else if (strcmp(name, "SPC 1000") == 0) {
        output = (input(360001) / 4) + 120000; // Pa
        sprintf(outputprint, "%G", output);
    }/*
         * KP 125 is a pressure  sensor
         * Range: 40 to 115 kPa
         * Output voltage range - 0.5 to 4.5 V
         */

    else if (strcmp(name, "KP 125") == 0) {
        tempInput = ((input(401) / 100) + 0.5); // sensor output in volt
        output = ((tempInput - 0.5) * 75 / 4) + 40; // kPa
        sprintf(outputprint, "%G", output);
    }/*
         * ADXL335 is a 3 axis accelerometer
         * For input voltage = 3.6 V, output voltage = 1.8 V for 0g
         * Sensitivity = 360mV/g
         * Output range: -3g to +3g i.e., 720mV to 2880 mV
         */

    else if (strcmp(name, "ADXL335") == 0) {
        tempInput = ((input(2161)) + 720); // sensor output in millivolt
        output = ((tempInput - 720) * 6 / 2160) - 3; // g
        sprintf(outputprint, "%G", output);
    }/*
         * ADXL345 is an accelerometer
         * For input voltage = 2.5 V and for +/- 4g,
         * Output LSB range:
         * X axis: 25 to 270
         * Y axis: -270 to 25
         * Z axis: 38 to 438
         */

    else if (strcmp(name, "ADXL345") == 0) {
        tempInput = ((input(246)) + 25); // sensor output in LSB
        output = ((tempInput - 25) * 8 / 245) - 4; // g
        sprintf(tempoutput, "%G", output);
        strcat(outputprint,tempoutput);
		strcat(outputprint,";");

        tempInput = ((input(296)) - 270); // sensor output in LSB
        output = ((tempInput + 270) * 8 / 295) - 4; // g
        sprintf(tempoutput, "%G", output);
        strcat(outputprint,tempoutput);
        strcat(outputprint,";");

        tempInput = ((input(401)) + 38); // sensor output in LSB
        output = ((tempInput - 38) * 8 / 400) - 4; // g
        sprintf(tempoutput, "%G", output);
        strcat(outputprint,tempoutput);
    }/* 
         * ADIS16266 is a gyroscope sensor
         * Output range in degrees / s - +/- 14000
         * Output range in decimal - +/- 3357
         * 
         */
    else if (strcmp(name, "ADIS16266") == 0) {
        tempInput = ((input(6715)) - 3357); // sensor output in LSB
        output = ((tempInput) * 14000 / 3357); // degree / s
        sprintf(outputprint, "%G", output);    
    }/* 
         * ADIS16266 is a gyroscope sensor
         * Output range in degrees / s - +/- 14000
         * Output range in decimal - +/- 3357
         * 
         */
    else if (strcmp(name, "ADIS16266") == 0) {
        tempInput = ((input(6715)) - 3357); // sensor output in LSB
        output = ((tempInput) * 14000 / 3357); // degree / s 
        sprintf(outputprint, "%G", output);   
    }/*
         * L3GD20 is a 3 axis gyroscope sensor
         * Output range in degrees /s: +/- 250,500,2000
         */

    else if (strcmp(name, "L3GD20") == 0) {
        output = input(501) - 250; // sensor output in LSB
        sprintf(outputprint, "%G", output);
    }/*
         * HTUD21D(F) is a humidity sensor
         * The relative humidity is in percentage
         * The output is in 16 bits and the conversion range from -6% to 118%
         */

    else if (strcmp(name, "HTUD21D(F)") == 0) {
        output = -6 + (125 * input(65536) / 65536); // sensor output in %
        sprintf(outputprint, "%G", output);
    }/*
         * SHT1x is a humidity sensor
         * The relative humidity is in percentage
         * The output is in 12 bits and the conversion may range from -2% to 106%
         */

    else if (strcmp(name, "SHT1x") == 0) {
        tempInput = input(3180) + 56;
        output = (tempInput * 0.0367) - 2.0468 - (tempInput * tempInput * 0.0000015955); // sensor output in %
        sprintf(outputprint, "%G", output);
    }
    
    printf("\t Output: %s", outputprint);
}

int main(void) {
    char sensorname[20];
    printf("Enter the sensor name without any spaces: ");

    scanf("%s", &sensorname);
    while (1) {
        convert(sensorname);
        sleep(1);
    }
}

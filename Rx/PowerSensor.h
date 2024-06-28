/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

// User *may* modify this file if he knows what he is doing

#pragma once
#include <Arduino.h>

#define PS_MODULE_NAME     "PowerSensor"
#define PS_MODULE_VERSION  "v1.1.0"

class PowerSensor {
    private:
        // ADC1 gpio that will be used for ADC Continuous mode
        uint8_t mAdcGpio[1]; // we use only one ADC input to sense the power supply but analogContinuous() expects an array
        static const uint8_t ADC_GPIOS_COUNT = 1;

        // Define at which frequency Read() will return values, in Hz
        uint32_t mReadSamplesFrequency;

        // Set the resolution to 9-12 bits
        uint8_t mResolution;

        // Define how many conversion per pin will happen and reading the data will be and average of all conversions
        uint32_t mConversions;

        // Set sampling frequency of ADC in Hz, minimum 20 kHz
        uint32_t mAdcFrequency;

        // Result structure for ADC Continuous reading
        adc_continuous_data_t *mResult = NULL;

        bool mConversionRunning;
        
    public:
        PowerSensor(uint8_t adc_gpio, uint32_t read_samples_frequency, uint8_t resolution=12, uint32_t conversions=50, uint32_t adc_frequency=20000);
        void Config(void);
        bool Read(uint16_t *avg_raw_value_out, uint16_t *avg_millivolts_out);
        bool ReadRaw(uint16_t *avg_raw_value_out);
        bool ReadVoltage(const float calibration, uint16_t *avg_millivolts_out);
};

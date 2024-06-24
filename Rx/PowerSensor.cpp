/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#include "PowerSensor.h"

static volatile bool ADC_conversion_complete; // Flag which will be set by the ISR on_AdcComplete() when conversion is done

// ISR Function that will be triggered when ADC conversion is done
void ARDUINO_ISR_ATTR on_AdcComplete(void) {
    ADC_conversion_complete = true;
}

// Continuous sampling - ONLY ADC1 pins are supported (GPIO32 - GPIO39)
PowerSensor::PowerSensor(uint8_t adc_gpio, uint32_t read_samples_frequency, uint8_t resolution, uint32_t conversions, uint32_t adc_frequency) {
    // only ADC1 gpios are supported (GPIO32 - GPIO39)
    mAdcGpio[0]=adc_gpio;
    // Define at which frequency Read() will return values, in Hz
    mReadSamplesFrequency=read_samples_frequency;
    // Set the resolution to 9-12 bits
    mResolution=resolution;
    // Define how many conversion per pin will happen and reading the data will be and average of all conversions
    mConversions=conversions;
    // Set sampling frequency of ADC in Hz, minimum 20 kHz
    mAdcFrequency=adc_frequency;

    ADC_conversion_complete=false;
    mConversionRunning=false;
}

void PowerSensor::Config(void) {
    // Optional for ESP32: Set the resolution to 9-12 bits (default is 12 bits)
    analogContinuousSetWidth(mResolution);

    // Optional: Set different attenuation (default is ADC_11db)
    analogContinuousSetAtten(ADC_11db);

    // Setup ADC Continuous with following input:
    // array of pins, count of the pins, how many conversions per pin in one cycle will happen, sampling frequency, callback function
    analogContinuous(mAdcGpio, ADC_GPIOS_COUNT, mConversions, mAdcFrequency, &on_AdcComplete);
    // ADC configured but not started yet : call analogContinuousStart/Stop to control it
}

// return raw ADC value and raw voltage in mV
bool PowerSensor::Read(uint16_t *avg_raw_value_out, uint16_t *avg_millivolts_out) {
    bool retval=false;
    static const unsigned long Period_ms=1000/mReadSamplesFrequency;

    static unsigned long Resume_time=0;
    if (millis()>=Resume_time)
        if (!mConversionRunning) {
            analogContinuousStart(); // resume ADC continuous conversions
            mConversionRunning=true;
        }

    if (ADC_conversion_complete) {
        ADC_conversion_complete = false;
        // Read data from ADC
        if (analogContinuousRead(&mResult, 0)) {
            analogContinuousStop(); // stop ADC continuous conversions for now
            mConversionRunning=false;
            *avg_raw_value_out=mResult[0].avg_read_raw;
            *avg_millivolts_out=mResult[0].avg_read_mvolts;
            Resume_time=millis()+Period_ms; // resume ADC continuous conversions after Period_ms
            retval=true;
        }
    }
    return retval;
}

// return raw ADC value, max value depends on mResolution
bool PowerSensor::ReadRaw(uint16_t *avg_raw_value_out) {
    uint16_t ignored=0;
    return Read(avg_raw_value_out, &ignored);
}

// return calibrated voltage in mV, using given calibration coef
// (give calibration=1 to get the raw voltage)
bool PowerSensor::ReadVoltage(const float calibration, uint16_t *avg_millivolts_out) {
    uint16_t ignored=0;
    uint16_t raw_voltage=0;
    bool retval=Read(&ignored, &raw_voltage);
    if (retval)
        *avg_millivolts_out=raw_voltage*calibration;
    return retval;
}

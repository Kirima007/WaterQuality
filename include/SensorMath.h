#pragma once
#include <Arduino.h>

class SensorMath {
public:
    static const float TARGET_PPT;

    // EC & Salinity
    static float calculate(float volt, float tempC, float alpha, float beta);
    static float calculateEC(float volt, float tempC, float alpha, float beta);
    
    // pH
    static float calculatePH(float volt, float tempC, float alpha, float beta);
    
    // DO
    static float calculateDO(float volt, float tempC, float alpha, float beta);

    static bool computeAlphaBeta(uint8_t currentParam,
                                float v1, float t1, 
                                float v2, float t2, 
                                float &alphaOut, float &betaOut);

    // Capture stable value over time
    static bool captureStableValue(uint8_t currentParam, float &capturedVolt, float &capturedTemp);

private:
    static float calcEC25(float volt, float tempC);
    static float calcPHBase(float volt, float tempC);
    static float calcDOBase(float volt, float tempC);
};
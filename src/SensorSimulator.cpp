#include "SensorSimulator.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr uint8_t LARGE_PIN=25, SMALL_PIN=26, OUTSIDE_PIN=19, PWM_CHANNEL=0;
constexpr uint32_t STALE_MS=30000;
constexpr float VCC=3.3f, R_FIXED=10000.0f, R_25=10000.0f, BETA=3950.0f, T_25_K=298.15f;

float calibrateF(float requestedF,int channel) {
    static constexpr float commanded[]={65.0f,90.0f,120.0f};
    static constexpr float measured[3][3]={
        {72.1f,96.7f,128.7f},
        {72.8f,98.6f,130.6f},
        {70.9f,96.7f,130.6f}
    };
    const float* readings=measured[channel];
    const int segment=requestedF<=readings[1]?0:1;
    const float fraction=(requestedF-readings[segment])/(readings[segment+1]-readings[segment]);
    return commanded[segment]+fraction*(commanded[segment+1]-commanded[segment]);
}
float voltageForF(float f) {
    const float k=(f-32.0f)*(5.0f/9.0f)+273.15f;
    const float r=R_25*expf(BETA*(1.0f/k-1.0f/T_25_K));
    return VCC*r/(R_FIXED+r);
}
uint8_t to8bit(float voltage) {
    const float scaled=voltage*255.0f/VCC;
    if(scaled<=0.0f)return 0;
    if(scaled>=255.0f)return 255;
    return static_cast<uint8_t>(scaled+0.5f);
}
}

void SensorSimulator::begin(Status& status) {
    _status = &status;
    // Outputs are enabled only when replay sends its first valid F command.
    Serial.println("READY HvacMonitor replay: F,largeF,smallF,outsideF");
}
void SensorSimulator::setZero() {
    dacWrite(LARGE_PIN,0);
    dacWrite(SMALL_PIN,0);
    ledcWrite(PWM_CHANNEL,0);
}
void SensorSimulator::process(char* line) {
    if(strncmp(line,"F,",2)!=0) {
        Serial.println("ERR expected F,large,small,outside");
        return;
    }
    float f[3];
    char* p=line+2;
    for(int i=0;i<3;++i) {
        char* end=nullptr;
        f[i]=strtof(p,&end);
        if(end==p||!isfinite(f[i])||f[i]<-20.0f||f[i]>200.0f) {
            Serial.println("ERR invalid temperature (-20..200 F)");
            return;
        }
        if(i<2) {
            if(*end!=','){Serial.println("ERR missing comma");return;}
            p=end+1;
        } else if(*end!='\0'&&*end!='\r') {
            Serial.println("ERR trailing characters");
            return;
        }
    }
    const uint8_t v[3]={
        to8bit(voltageForF(calibrateF(f[0],0))),
        to8bit(voltageForF(calibrateF(f[1],1))),
        to8bit(voltageForF(calibrateF(f[2],2)))
    };
    if(!_initialized) {
        ledcSetup(PWM_CHANNEL,5000,8);
        ledcAttachPin(OUTSIDE_PIN,PWM_CHANNEL);
        _initialized=true;
    }
    dacWrite(LARGE_PIN,v[0]);
    dacWrite(SMALL_PIN,v[1]);
    ledcWrite(PWM_CHANNEL,v[2]);
    _active=true;
    for (int i = 0; i < 3; ++i) _requestedF[i] = f[i];
    _lastUpdate=millis();
    Serial.printf("OK %.2f %.2f %.2f DAC=%u,%u PWM=%u\n",f[0],f[1],f[2],v[0],v[1],v[2]);
}
void SensorSimulator::update() {
    // Bound the work per loop so serial input cannot starve LVGL/network.
    int remaining=128;
    while(remaining-->0&&Serial.available()) {
        const char c=static_cast<char>(Serial.read());
        if(c=='\n') {
            _input[_used]='\0';
            process(_input);
            _used=0;
        } else if(_used<sizeof(_input)-1) {
            _input[_used++]=c;
        } else {
            _used=0;
            Serial.println("ERR line too long");
        }
    }
    if (_active && millis() - _lastSetpointPrint >= 1000) {
        _lastSetpointPrint = millis();
        Serial.printf("SIM,%.1f,%.1f,%.1f\n", _requestedF[0], _requestedF[1], _requestedF[2]);
    }
    if(_active&&millis()-_lastUpdate>STALE_MS) {
        _active=false;
        setZero();
        Serial.println("STALE outputs zero");
    }
}

void SensorSimulator::reportTemperatures() {
    if (!_status) return;
    const float f[3] = {
        _status->largePipeTemperature() * 1.8f + 32.0f,
        _status->smallPipeTemperature() * 1.8f + 32.0f,
        _status->outdoorTemperature() * 1.8f + 32.0f
    };
    int current[3];
    bool changed = false;
    for (int i = 0; i < 3; ++i) {
        current[i] = isfinite(f[i]) ? static_cast<int>(lroundf(f[i] * 10.0f)) : INT32_MIN;
        if (current[i] != _previous[i]) changed = true;
    }
    if (changed && _active) {
        Serial.printf("TEMP,%.1f,%.1f,%.1f\n", f[0], f[1], f[2]);
    }
    for (int i = 0; i < 3; ++i) _previous[i] = current[i];
}

#ifndef __FLASH_LED_H
#define __FLASH_LED_H

#include <HardwareTimer.h>

class FlashLedClass
{
public:
    FlashLedClass(uint32_t pin, TIM_TypeDef *instance) : _pin(pin), _stopped(true), _ptr(nullptr), _instance(instance), _pLedTicker(nullptr) {}
    ~FlashLedClass();
    bool begin(uint32_t milliseconds, uint32_t times);
    void stop();
    bool isStopped() const;
private:
    void _ledTickerCallback();
	uint8_t _pin;
    uint8_t _pinStatus;
    uint32_t _interval;
    uint32_t _times;
    bool _stopped;
    void *_ptr;
    TIM_TypeDef *_instance;
    HardwareTimer *_pLedTicker;
};

#endif
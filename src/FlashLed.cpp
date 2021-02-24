#include "FlashLed.h"
#include "wiring_digital.h"
#include "wiring_constants.h"
#include <Functional>
#include <cstdlib>
#include <FreeRTOS.h>

FlashLedClass::~FlashLedClass()
{
    this->_pLedTicker->~HardwareTimer();
    if (this->_ptr != nullptr);
    vPortFree(this->_ptr);
    digitalWrite(this->_pin, this->_pinStatus);
}

bool FlashLedClass::begin(uint32_t milliseconds, uint32_t times)
{
    if (!this->isStopped())
    {
        this->stop();
    }
    if (nullptr == this->_ptr)
    {
        this->_ptr = pvPortMalloc(sizeof(HardwareTimer));
        if (nullptr == this->_ptr)
            return false;
    }
    pinMode(this->_pin, OUTPUT);
    this->_pLedTicker = new (this->_ptr) HardwareTimer(this->_instance);
    this->_interval = milliseconds * 1000;
    this->_times = times * 2;
    this->_stopped = false;
    this->_pinStatus = digitalRead(this->_pin);
    this->_pLedTicker->setOverflow(this->_interval, MICROSEC_FORMAT);
    this->_pLedTicker->attachInterrupt(std::bind(&FlashLedClass::_ledTickerCallback, this));
    this->_pLedTicker->resume();
    return true;
}

void FlashLedClass::stop()
{
    this->_pLedTicker->~HardwareTimer();
    this->_pLedTicker = nullptr;
    digitalWrite(this->_pin, this->_pinStatus);
    this->_stopped = true;
}

bool FlashLedClass::isStopped() const
{
    return this->_stopped;
}

void FlashLedClass::_ledTickerCallback()
{
    if (this->_times > 0)
    {
        digitalToggle(this->_pin);
        if (--this->_times == 0)
        {
            this->stop();
        }
    }
    else
    {
        digitalToggle(this->_pin);
    }
}

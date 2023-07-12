#pragma once
#ifndef DSY_WS2812_H
#define DSY_WS2812_H

#include "per/tim_channel.h"

namespace daisy
{

/**
 * @brief Driver class for Ws2812 LEDs. Currently limited to 32 LEDs max
 *        and one instance used per daisy due to shared DMA buffer.
 */
class Ws2812
{
  public:
    static constexpr uint8_t kMaxNumLEDs = 64;

    struct Config
    {
        uint8_t num_leds;

        TimerHandle::Config::Peripheral timer_periph;
        TimChannel::Config::Channel     tim_channel;
        Pin                             tim_pin;

        uint32_t symbol_length_ns = 1250;
        uint32_t zero_high_ns     = 400;
        uint32_t one_high_ns      = 800;
    };

    Ws2812()  = default;
    ~Ws2812() = default;

    void Init(const Config& config);

    void Set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
    void Set(uint8_t idx, uint32_t color);
    void Fill(uint32_t color);
    void Clear();

    void Show();

    class Impl;

  private:
    Impl*   pimpl_;
    uint8_t num_leds_;
};

} // namespace daisy

#endif

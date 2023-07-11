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
    static constexpr uint8_t kMaxNumLEDs = 32;

    struct Config
    {
        uint8_t num_leds;

        TimerHandle::Config::Peripheral timer_periph;
        TimChannel::Config::Channel     tim_channel;
        Pin                             tim_pin;

        uint32_t prescaler = 8;

        // Percent of time pulse is "high" to represent zero and one.
        // Target total pulse length is ~1.225uS based on datasheet.
        float zero_high_pct = 0.29f;
        float one_high_pct  = 0.57f;
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

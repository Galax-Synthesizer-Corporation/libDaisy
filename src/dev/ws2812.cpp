#include "ws2812.h"
#include "sys/dma.h"
#include "sys/system.h"
#include <algorithm>

using namespace daisy;

namespace daisy
{
// 3 colors * 8 values per LED plus one leading and one trailing zero
static constexpr size_t kPwmOutBufSize = Ws2812::kMaxNumLEDs * 3 * 8 + 2;
static uint32_t DMA_BUFFER_MEM_SECTION pwm_out_buf[kPwmOutBufSize];

/** Private impl class for single static shared instance */
class Ws2812::Impl
{
  public:
    Impl()  = default;
    ~Impl() = default;

    void Init(const Ws2812::Config& config);

    void Set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b)
    {
        if(idx >= num_leds_)
            return;
        led_data_[idx][0] = r;
        led_data_[idx][1] = g;
        led_data_[idx][2] = b;
    }

    void Show();

  private:
    TimerHandle timer_;
    TimChannel  pwm_;

    uint8_t  num_leds_;
    uint32_t zero_period_;
    uint32_t one_period_;

    uint32_t* dma_buffer_;
    size_t    dma_buffer_size_;

    uint8_t led_data_[Ws2812::kMaxNumLEDs][3]; /**< RGB data */

    bool dma_ready_;

    static void OnTransferEnd(void* context)
    {
        Ws2812::Impl* pimpl = reinterpret_cast<Ws2812::Impl*>(context);
        pimpl->dma_ready_   = true;
        // pimpl->pwm_.Start();
    }

    void populateBits(uint8_t color_val, uint32_t* buff)
    {
        uint8_t mask = 0x80;
        for(int i = 0; i < 8; i++)
        {
            buff[i] = (color_val & mask) > 0 ? one_period_ : zero_period_;
            mask    = mask >> 1;
        }
    }

    bool isDMAReady() { return dma_ready_; }

    void fillDMABuffer()
    {
        for(uint32_t i = 0; i < num_leds_; i++)
        {
            /** Grab G, R, B for filling bytes */
            // TODO: Alt color order?
            uint8_t g = led_data_[i][1];
            uint8_t r = led_data_[i][0];
            uint8_t b = led_data_[i][2];

            size_t data_index = i * 3 * 8 + 1;
            populateBits(g, &dma_buffer_[data_index]);
            populateBits(r, &dma_buffer_[data_index + 8]);
            populateBits(b, &dma_buffer_[data_index + 16]);
        }
    }
};

static Ws2812::Impl impl;

} // namespace daisy

void Ws2812::Impl::Init(const Ws2812::Config& config)
{
    TimerHandle::Config tim_cfg;
    tim_cfg.periph = config.timer_periph;
    tim_cfg.dir    = TimerHandle::Config::CounterDir::UP;
    timer_.Init(tim_cfg);

    // TODO: use correct clock for the peripheral
    uint32_t prescaler         = 1;
    uint32_t tickspeed         = (System::GetPClk1Freq() * 2) / prescaler;
    uint32_t target_pulse_freq = 1e9 / config.symbol_length_ns;
    uint32_t period            = (tickspeed / target_pulse_freq) - 1;

    timer_.SetPrescaler(prescaler - 1);
    timer_.SetPeriod(period);

    TimChannel::Config pwm_cfg;
    pwm_cfg.tim      = &timer_;
    pwm_cfg.chn      = config.tim_channel;
    pwm_cfg.mode     = TimChannel::Config::Mode::PWM;
    pwm_cfg.polarity = TimChannel::Config::Polarity::HIGH;
    pwm_cfg.pin      = config.tim_pin;

    pwm_.Init(pwm_cfg);
    pwm_.SetPwm(0);

    num_leds_ = std::min(config.num_leds, Ws2812::kMaxNumLEDs);
    zero_period_
        = period * ((float)config.zero_high_ns / config.symbol_length_ns);
    one_period_
        = period * ((float)config.one_high_ns / config.symbol_length_ns);

    // TODO: Externally passable?
    dma_buffer_      = pwm_out_buf;
    dma_buffer_size_ = num_leds_ * 3 * 8 + 2;
    dma_ready_       = true;

    for(size_t i = 0; i < dma_buffer_size_; i++)
    {
        dma_buffer_[i] = 0;
    }
}

void Ws2812::Impl::Show()
{
    if(!isDMAReady())
        return;
    dma_ready_ = false;
    fillDMABuffer();
    pwm_.SetPwm(0);
    pwm_.StartDma(dma_buffer_, dma_buffer_size_, &Ws2812::Impl::OnTransferEnd, this);
}

// -------

void Ws2812::Init(const Config& config)
{
    pimpl_ = &impl;
    pimpl_->Init(config);
    num_leds_ = config.num_leds;
    Clear();
}

void Ws2812::Set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b)
{
    pimpl_->Set(idx, r, g, b);
}

void Ws2812::Set(uint8_t idx, uint32_t color)
{
    color = (color & 0x00FFFFFF);
    Set(idx, color >> 16, color >> 8, color);
}

void Ws2812::Fill(uint32_t color)
{
    for(uint8_t i = 0; i < num_leds_; i++)
    {
        Set(i, color);
    }
}

void Ws2812::Clear()
{
    Fill(0);
}

void Ws2812::Show()
{
    pimpl_->Show();
}

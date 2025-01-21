#include "hid/switch.h"
using namespace daisy;

void Switch::InitManual()
{
    Init(Pin());
    manual_input_ = true;
}

void Switch::Init(Pin        pin,
                  float      update_rate,
                  Type       t,
                  Polarity   pol,
                  GPIO::Pull pu)
{
    manual_input_ = false;
    last_update_  = System::GetNow();
    updated_      = false;
    state_        = 0x00;
    t_            = t;
    // Flip may seem opposite to logical direction,
    // but here 1 is pressed, 0 is not.
    flip_ = pol == POLARITY_INVERTED ? true : false;
    hw_gpio_.Init(pin, GPIO::Mode::INPUT, pu);
}
void Switch::Init(Pin pin, float update_rate)
{
    Init(pin,
         update_rate,
         TYPE_MOMENTARY,
         POLARITY_INVERTED,
         GPIO::Pull::PULLUP);
}

void Switch::Debounce()
{
    const bool new_val = hw_gpio_.Read();
    const bool on      = flip_ ? !new_val : new_val;
    DebounceManual(on);
}

void Switch::DebounceManual(bool on)
{
    last_input_ = on;

    // update no faster than 1kHz
    uint32_t now = System::GetNow();
    updated_     = false;

    if(now - last_update_ >= 1)
    {
        last_update_ = now;
        updated_     = true;

        // shift over, and introduce new state.
        state_ = (state_ << 1) | on;
        // Set time at which button was pressed
        if(state_ == 0x7f)
            rising_edge_time_ = System::GetNow();
    }
}

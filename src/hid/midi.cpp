#include "midi.h"
#include <string.h>

namespace daisy
{
static constexpr size_t kDefaultMidiRxBufferCapacity = 256;
static constexpr size_t kDefaultMidiTxBufferCapacity = 256;

static uint8_t DMA_BUFFER_MEM_SECTION
    default_midi_rx_buffer[kDefaultMidiRxBufferCapacity];

static uint8_t default_midi_tx_buffer[kDefaultMidiTxBufferCapacity];

MidiUartTransport::Config::Config()
{
    periph             = UartHandler::Config::Peripheral::USART_1;
    rx                 = {DSY_GPIOB, 7};
    tx                 = {DSY_GPIOB, 6};
    rx_buffer          = default_midi_rx_buffer;
    rx_buffer_capacity = kDefaultMidiRxBufferCapacity;
    tx_buffer          = default_midi_tx_buffer;
    tx_buffer_capacity = kDefaultMidiTxBufferCapacity;
}

MidiTxMessage::MidiTxMessage()
{
    size = 0;
    std::fill(data, data + kMaxDataSize, 0);
}

MidiTxMessage MidiTxMessage::NoteOn(uint8_t ch, uint8_t nn, uint8_t vel)
{
    MidiTxMessage msg;
    msg.size = 3;
    // status byte
    msg.data[0] = 0x90 | (ch & 0x0F);
    // note num
    msg.data[1] = nn & 0x7F;
    // velocity
    msg.data[2] = vel & 0x7F;
    return msg;
}

MidiTxMessage MidiTxMessage::NoteOff(uint8_t ch, uint8_t nn, uint8_t vel)
{
    MidiTxMessage msg;
    msg.size = 3;
    // status byte
    msg.data[0] = 0x80 | (ch & 0x0F);
    // note num
    msg.data[1] = nn & 0x7F;
    // velocity
    msg.data[2] = vel & 0x7F;
    return msg;
}

MidiTxMessage MidiTxMessage::PitchBend(uint8_t ch, int16_t bend)
{
    MidiTxMessage msg;
    msg.size = 3;
    bend += 8192;
    // status byte
    msg.data[0] = 0xE0 | (ch & 0x0F);
    // lsb
    msg.data[1] = bend & 0x7F;
    // msb
    msg.data[2] = (bend >> 7) & 0x7F;
    return msg;
}

MidiTxMessage MidiTxMessage::SystemRealtimeClock()
{
    MidiTxMessage msg;
    msg.data[0] = 0xf8;
    msg.size    = 1;
    return msg;
}
MidiTxMessage MidiTxMessage::SystemRealtimeStart()
{
    MidiTxMessage msg;
    msg.data[0] = 0xfa;
    msg.size    = 1;
    return msg;
}
MidiTxMessage MidiTxMessage::SystemRealtimeStop()
{
    MidiTxMessage msg;
    msg.data[0] = 0xfc;
    msg.size    = 1;
    return msg;
}
MidiTxMessage MidiTxMessage::SystemExclusive(const uint8_t* data, size_t size)
{
    if(size > SYSEX_BUFFER_LEN)
        return MidiTxMessage();

    // Sysex data + start byte + stop byte
    MidiTxMessage msg;
    msg.size = size + 2;
    // start byte
    msg.data[0] = 0xF0;
    // data
    memcpy(msg.data + 1, data, size);
    // stop byte
    msg.data[size + 1] = 0xF7;
    return msg;
}

} // namespace daisy

#pragma once
#include <stdint.h>
#include "UiEventQueue.h"
#include "../sys/system.h"

namespace daisy
{

enum PotTrackingMode
{
    // Always post changes immediately
    Immediate,
    // Wait until value gets close to explicitly set
    // "internal value" to begin tracking again
    Pickup
};

/** @brief A potentiometer monitor that generates events in a UiEventQueue
 *  @author jelliesen
 *  @ingroup ui
 *
 *  This class monitors a number of potentiometers and detects pot movements.
 *  When a movement is detected, an event is added to a UiEventQueue.
 *  Pots can be either "idle" or "moving" in which case different dead bands
 *  are applied to them. The current state and value of a pot can be requested at any time.
 *
 *  This class can monitor an arbitrary number of potentiometers, as configured by its
 *  template argument `numPots`. Each of the pots is identified by an ID number from
 *  `0 .. numPots - 1`. This number will also be used when events are posted to the
 *  UiEventQueue. It's suggested to define an enum in your project like this:
 *
 *      enum PotId { potA = 0, potB = 1, potC = 2 };
 *
 *  In different projects, diffent ways of reading the potentiometer positions will be
 *  used. That's why this class uses a generic backend that you'll have to write.
 *  The BackendType class will provide the source data for each potentiometer.
 *  An instance of this backend must be supplied via the constructor.
 *  It must implement the following public function via which the PotMonitor
 *  will request the current value of the potentiometer in the range `0 .. 1`:
 *
 *      float GetPotValue(uint16_t potId);
 *
 *   @tparam BackendType     The class type of the backend that will supply pot values.
 *   @tparam numPots         The number of pots to monitor.
 */
template <typename BackendType, uint32_t numPots>
class PotMonitor
{
  public:
    PotMonitor()
    : queue_(nullptr),
      backend_(nullptr),
      deadBand_(1.0 / (1 << 12)),
      deadBandIdle_(1.0 / (1 << 10)),
      timeout_(0)
    {
    }

    /** Initialises the PotMonitor.
     * @param queueToAddEventsTo    The UiEventQueue to which events should be posted.
     * @param backend                The backend that supplies the current value of each potentiometer.
     * @param idleTimeoutMs           When the pot is currently moving, but no event is generated over
     *                              "idleTimeoutMs", the pot enters the idle state.
     * @param deadBandIdle             The dead band that must be exceeded before a movement is detected
     *                              when the pot is currently idle.
     * @param deadBand                 The dead band that must be exceeded before a movement is detected
     *                              when the pot is currently moving.
     */
    void Init(UiEventQueue& queueToAddEventsTo,
              BackendType&  backend,
              uint16_t      idleTimeoutMs = 500,
              float         deadBandIdle  = 1.0 / (1 << 10),
              float         deadBand      = 1.0 / (1 << 12))
    {
        queue_        = &queueToAddEventsTo;
        deadBand_     = deadBand;
        backend_      = &backend;
        deadBandIdle_ = deadBandIdle;
        timeout_      = idleTimeoutMs;

        for(uint32_t i = 0; i < numPots; i++)
        {
            initialized_[i]      = false;
            lastValue_[i]        = 0.0;
            timeoutCounterMs_[i] = 0;
            pickupTarget_[i]     = 0.0f;
            tracking_[i]         = true;
        }

        lastCallSysTime_ = System::GetNow();
    }

    void SetTrackingMode(PotTrackingMode mode)
    {
        if(mode != tracking_mode_)
        {
            tracking_mode_ = mode;
            if(mode == PotTrackingMode::Immediate)
            {
                for(uint32_t i = 0; i < numPots; i++)
                {
                    tracking_[i] = true;
                }
            }
        }
    }

    /** Checks the value of each pot and generates messages for the UIEventQueue.
     *  Call this at regular intervals, ideally from your main() idle loop.
     */
    void Process()
    {
        const auto now      = System::GetNow();
        const auto timeDiff = now - lastCallSysTime_;
        lastCallSysTime_    = now;

        for(uint32_t i = 0; i < numPots; i++)
            ProcessPot(i, backend_->GetPotValue(i), timeDiff);
    }

    /** Returns true, if the requested pot is currently being moved.
     *  @param potId    The unique ID of the potentiometer (< numPots)
     */
    bool IsMoving(uint16_t potId) const
    {
        if(potId >= numPots)
            return false;
        else
            return timeoutCounterMs_[potId] < timeout_;
    }

    bool IsTracking(bool potId) const
    {
        if(potId >= numPots)
            return false;
        else
            return tracking_[potId];
    }

    /** For a given potentiometer, this will return the last value that was
     *  posted to the UiEventQueue.
     *  @param potId    The unique ID of the potentiometer (< numPots)
     */
    float GetCurrentPotValue(uint16_t potId) const
    {
        if(potId >= numPots)
            return -1.0f;
        else
            return lastValue_[potId];
    }

    void SetPickupTarget(uint16_t pot_id, float value)
    {
        if(pot_id < numPots && tracking_mode_ == PotTrackingMode::Pickup)
        {
            float diff            = fabsf(lastValue_[pot_id] - value);
            pickupTarget_[pot_id] = value;
            tracking_[pot_id]     = diff <= deadBand_;

            // if the pot is not tracking but it is currently moving,
            // force it to stop and send message
            if(!tracking_[pot_id] && IsMoving(pot_id))
            {
                timeoutCounterMs_[pot_id] = timeout_;
                queue_->AddPotActivityChanged(pot_id, false);
            }
        }
    }

    /** Returns the BackendType that is used by the monitor. */
    BackendType& GetBackend() { return backend_; }

    /** Returns the number of pots that are monitored by this class. */
    uint16_t GetNumPotsMonitored() const { return numPots; }

  private:
    /** Process a potentiometer and detect movements - or
     *  flags the pot as "idle" when no movement is detected for
     *  a longer period of time.
     *  @param id            The unique ID of the potentiometer (< numPots)
     *  @param value        The new value in the range 0..1
     *  @param timeDiffMs    The time in ms since the last call
     */
    void ProcessPot(uint16_t id, float value, uint32_t timeDiffMs)
    {
        bool tracking
            = tracking_[id] || tracking_mode_ == PotTrackingMode::Immediate;

        // currently moving?
        if(!initialized_[id])
        {
            initialized_[id] = true;
            lastValue_[id]   = value;
            queue_->AddPotMoved(id, value);
        }
        else if(tracking && timeoutCounterMs_[id] < timeout_)
        {
            // check if pot has left the deadband. If so, add a new message
            // to the queue.
            float delta = lastValue_[id] - value;
            if((delta > deadBand_) || (delta < -deadBand_))
            {
                lastValue_[id] = value;
                queue_->AddPotMoved(id, value);
                timeoutCounterMs_[id] = 0;
            }
            // no movement, increment timeout counter
            else
            {
                timeoutCounterMs_[id] += timeDiffMs;
                // post activity changed event after timeout expired.
                if(timeoutCounterMs_[id] == timeout_)
                {
                    queue_->AddPotActivityChanged(id, false);
                }
            }
        }
        // tracking but not moving right now
        else if(tracking)
        {
            // check if pot has left the idle deadband. If so, add a new message
            // to the queue and restart the timeout
            float delta = lastValue_[id] - value;
            if((delta > deadBandIdle_) || (delta < -deadBandIdle_))
            {
                lastValue_[id] = value;
                queue_->AddPotActivityChanged(id, true);
                queue_->AddPotMoved(id, value);
                timeoutCounterMs_[id] = 0;
            }
        }
        // not tracking
        else
        {
            // Update tracking state
            bool passthrough = (lastValue_[id] < pickupTarget_[id]
                                && value > pickupTarget_[id])
                               || (lastValue_[id] > pickupTarget_[id]
                                   && value < pickupTarget_[id]);
            tracking_[id]
                = passthrough || fabsf(value - pickupTarget_[id]) <= deadBand_;
            if(tracking_[id])
            {
                lastValue_[id] = value;
                queue_->AddPotActivityChanged(id, true);
                queue_->AddPotMoved(id, value);
                timeoutCounterMs_[id] = 0;
            }
        }
    }

    PotMonitor(const PotMonitor&)            = delete;
    PotMonitor& operator=(const PotMonitor&) = delete;

    UiEventQueue*   queue_;
    BackendType*    backend_;
    PotTrackingMode tracking_mode_;
    float           deadBand_;
    float           deadBandIdle_;
    uint16_t        timeout_;
    bool            initialized_[numPots];
    float           lastValue_[numPots];
    float           pickupTarget_[numPots];
    bool            tracking_[numPots];
    uint16_t        timeoutCounterMs_[numPots];
    uint32_t        lastCallSysTime_;
};

} // namespace daisy

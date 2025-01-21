#include "logger_impl.h"
#include <stm32h750xx.h>

using namespace daisy;

// The below will not link correctly unless the application Makefile
// or make invocation includes SEMIHOSTING=1

extern "C" void initialise_monitor_handles(void);

void daisy::LoggerImpl<LOGGER_SEMIHOST>::Init()
{
    if (System::IsDebuggerAttached())
        initialise_monitor_handles();
}

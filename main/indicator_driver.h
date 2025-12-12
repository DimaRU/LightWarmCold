//
// indicator_driver.h
//

// Pairing indication signals
enum class SignalIndicator : short
{
    startup,            // Powered on
    connected,          // Thread/Wifi connection established
    commissioningOpen,  // Commissioning window opened
    commissioningStart, // Commissioning session started
    commissioningStop,  // Commissioning complete/failed
    commissioningClose, // Commissioning window closed
    identificationStart,
    identificationStop
};

void signalIndicator(enum SignalIndicator signal);
void indicator_driver_init();

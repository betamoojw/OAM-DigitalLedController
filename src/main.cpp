#include "DigitalLedModule.h"
#include "OpenKNX.h"
#include "hardware.h"

const uint8_t firmwareRevision = 1;

void setup()
{
    Serial.begin(115200);
    openknx.init(firmwareRevision);
    // Keep the LED module's flash ID stable across ETS module changes.
    openknx.addModule(1, digitalLedModule);
    openknx.setup();
}

void loop()
{
    openknx.loop();
}

#if defined(OPENKNX_DUALCORE)
void setup1()
{
    openknx.setup1();
}

void loop1()
{
    openknx.loop1();
}
#endif

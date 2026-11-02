#include "DigitalLedModule.h"
#include "FunctionBlocksModule.h"
#include "Logic.h"
#include "OpenKNX.h"
#include "VirtualButtonModule.h"
#include "hardware.h"

const uint8_t firmwareRevision = 0;

void setup()
{
    Serial.begin(115200);
    openknx.init(firmwareRevision);
    // These stable flash IDs are independent of the generated ETS_ModuleId_* values.
    openknx.addModule(1, digitalLedModule);
    openknx.addModule(2, openknxFunctionBlocksModule);
    openknx.addModule(3, openknxLogic);
    openknx.addModule(4, openknxVirtualButtonModule);
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

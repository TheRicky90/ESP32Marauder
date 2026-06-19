#ifndef BLE_JAMMER_H
#define BLE_JAMMER_H

#include <Arduino.h>

namespace BleJammerTool {

    // Funzioni pubbliche esposte a Marauder
    void runModuleLoop();
    void drawFullInterface();
    void drawInterfaceStatus(String currentLog);
    void updateRadioState();

} // namespace BleJammerTool

#endif // BLE_JAMMER_H

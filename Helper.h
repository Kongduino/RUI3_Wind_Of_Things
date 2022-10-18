long startTime;
double txDelay = 60000; // 1 minute. Too short but for now will do.
double myFreq = 868125000;
uint16_t mySF = 12, myBW = 125, myCR = 0, myTX = 22;
uint32_t lastCount = 0;

struct particleSensorState_t {
  uint16_t avgPM25 = 0;
  uint16_t measurements[5] = {0, 0, 0, 0, 0};
  uint8_t measurementIdx = 0;
  boolean valid = false;
  uint32_t Count = 0;
};
particleSensorState_t state;

void hexDump(uint8_t* buf, uint16_t len) {
  char alphabet[17] = "0123456789abcdef";
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
  Serial.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\n"));
  for (uint16_t i = 0; i < len; i += 16) {
    if (i % 128 == 0)
      Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
    char s[] = "|                                                | |                |\n";
    uint8_t ix = 1, iy = 52;
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j < len) {
        uint8_t c = buf[i + j];
        s[ix++] = alphabet[(c >> 4) & 0x0F];
        s[ix++] = alphabet[c & 0x0F];
        ix++;
        if (c > 31 && c < 128) s[iy++] = c;
        else s[iy++] = '.';
      }
    }
    uint8_t index = i / 16;
    if (i < 256) Serial.write(' ');
    Serial.print(index, HEX); Serial.write('.');
    Serial.print(s);
  }
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
}

bool isValidHeader(char *serialRxBuf) {
  bool headerValid = serialRxBuf[0] == 0x16 && serialRxBuf[1] == 0x11 && serialRxBuf[2] == 0x0B;
  if (!headerValid) {
    Serial.println("Received message with invalid header.");
  }
  return headerValid;
}

void parseState(char *serialRxBuf, particleSensorState_t* state) {
  /**
             MSB  DF 3     DF 4  LSB
     uint16_t = xxxxxxxx xxxxxxxx
  */
  const uint16_t pm25 = (serialRxBuf[5] << 8) | serialRxBuf[6];
  Serial.printf("Received PM 2.5 reading: %d\n", pm25);
  state->measurements[state->measurementIdx] = pm25;
  state->measurementIdx = (state->measurementIdx + 1) % 5;
  if (state->measurementIdx == 0) {
    float avgPM25 = 0.0f;
    for (uint8_t i = 0; i < 5; ++i) avgPM25 += state->measurements[i] / 5.0f;
    state->avgPM25 = avgPM25;
    state->valid = true;
    state->Count += 1;
    Serial.printf("New Avg PM25: %d\n", state->avgPM25);
  }
}

bool isValidChecksum(char *serialRxBuf) {
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < 20; i++) checksum += serialRxBuf[i];
  if (checksum != 0) {
    Serial.printf("Received message with invalid checksum. Expected: 0. Actual: %d\n", checksum);
  }
  return checksum == 0;
}

void sendData() {
  digitalWrite(WB_LED2, 255);
  Serial.printf(
    "\n+===============================================+\nSending latest measurements:\n  * %d, %d\n+===============================================+\n",
    state.avgPM25, lastCount
  );
  char payload[64];
  sprintf(payload, "{\"UUID\": \"%08X\",\"cmd\":\"PM2.5\",\"PM25\":%d}", lastCount++, state.avgPM25);
  Serial.printf("P2P send %s: %s\r\n", payload, api.lorawan.psend(strlen(payload), (uint8_t*)payload) ? "Success" : "Fail");
  startTime = millis();
#ifdef __RAKBLE_H__
  api.ble.uart.write((uint8_t*)payload, strlen(payload));
#endif
  delay(500);
  digitalWrite(WB_LED2, 0);
}

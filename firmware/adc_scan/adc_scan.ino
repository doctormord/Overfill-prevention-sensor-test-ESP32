/*
  ADC scan for the TTGO T-Display.

  Prints every ADC1 pin at once. Connect the divider and watch which pin
  moves -- faster than reading a pinout, and it catches a board whose
  silkscreen disagrees with its wiring.

  ADC2 (GPIO 0, 2, 4, 12-15, 25-27) is deliberately not in the list: it is
  unusable while WiFi is active, so a pin that works here and fails later
  would be the worst possible outcome.

  Expectation with 100k/10k on 19 V:  about 1730 mV / about 2150 counts
*/

const uint8_t pins[] = {32, 33, 34, 35, 36, 37, 38, 39};

void setup() {
  Serial.begin(115200);
  delay(300);
  for (uint8_t i = 0; i < sizeof(pins); i++) {
    analogSetPinAttenuation(pins[i], ADC_11db);
  }
  Serial.println("\nADC1 scan  (raw counts / mV)");
}

void loop() {
  for (uint8_t i = 0; i < sizeof(pins); i++) {
    uint32_t raw = 0, mv = 0;
    for (uint8_t n = 0; n < 8; n++) {
      raw += analogRead(pins[i]);
      mv  += analogReadMilliVolts(pins[i]);
    }
    Serial.printf("GPIO%-2d %5lu / %5lu mV   ", pins[i], raw / 8, mv / 8);
  }
  Serial.println();
  delay(500);
}

#include <Arduino.h>
#include <U8g2lib.h>
#include <esp_system.h>
#include <math.h>

// LCM12864D-V1.0: interface KS0108 de escrita, RW fisicamente no GND.
// DB0..DB7=4..11, CS1=12, CS2=13, RST=14, RS=15, E=16.
// Versao WHITE: VDD=5V, CS1/CS2 ativos LOW, VIH minimo=2V (folha do vendedor).
// USB OTG (GPIO19/20) fica livre. Potenciometro 20k entre VDD/VEE, cursor VO.
constexpr uint8_t CS_LEFT = 12, CS_RIGHT = 13, RESET_LCD = 14;
constexpr uint8_t DC_LCD = 15, ENABLE_LCD = 16, BUTTON = 0;
constexpr uint32_t FRAME_MS = 50, SCENE_MS = 12000;
constexpr float TAU = 6.28318530718f;
constexpr uint8_t SCENES = 10;
U8G2_KS0108_128X64_F lcd(U8G2_R0, 4, 5, 6, 7, 8, 9, 10, 11,
                        ENABLE_LCD, DC_LCD, CS_LEFT, CS_RIGHT,
                        U8X8_PIN_NONE, RESET_LCD);
const char *const names[SCENES] = {
    "OLA, HUMANO!", "RADAR", "CUBO 3D", "VIAGEM ESPACIAL", "PONG / DEMO",
    "ONDAS / DEMO", "AUDIO / DEMO", "RELOGIO / DEMO", "TESTE PIXELS", "TESTE CS1 / CS2"};
uint8_t scene = 0;
bool automatic = true, paused = false, inverted = false, rotated = false;
// Ordem confirmada visualmente com CS ativo LOW: esquerda=GPIO12, direita=GPIO13.
bool csActiveLow = true, swapHalves = false;
uint32_t animationMs = 0, sceneElapsed = 0, lastTick = 0, lastFrame = 0;
uint32_t frameCount = 0, transferUs = 0, bufferHash = 0;
u8x8_msg_cb baseGpio;

// Mantem a biblioteca intacta, com tempos conservadores para fios de bancada.
uint8_t lcdGpio(u8x8_t *u8x8, uint8_t msg, uint8_t arg, void *ptr) {
  if (msg == U8X8_MSG_GPIO_CS || msg == U8X8_MSG_GPIO_CS1) {
    bool left = msg == U8X8_MSG_GPIO_CS;
    uint8_t pin = (left != swapHalves) ? CS_LEFT : CS_RIGHT;
    digitalWrite(pin, (arg != 0) != csActiveLow);
    return 1;
  }
  if (msg == U8X8_MSG_DELAY_NANO) {
    if (arg) delayMicroseconds(1);
    return 1;
  }
  return baseGpio(u8x8, msg, arg, ptr);
}

void centered(const char *s, int y) {
  lcd.drawStr(max(0, (128 - (int)lcd.getStrWidth(s)) / 2), y, s);
}

void header() {
  lcd.setFont(u8g2_font_5x8_tr);
  lcd.drawStr(2, 8, names[scene]);
  char s[8];
  snprintf(s, sizeof(s), "%02u %c", scene + 1, paused ? 'P' : automatic ? 'A' : 'M');
  lcd.drawStr(104, 8, s);
  lcd.drawHLine(0, 10, 128);
}

void robot(uint32_t t) {
  header();
  int y = 19 + (int)roundf(sinf(t * .004f) * 2);
  lcd.drawRFrame(40, y, 48, 27, 5);
  lcd.drawLine(64, y, 64, y - 4);
  lcd.drawDisc(64, y - 6, 2);
  lcd.drawBox(36, y + 9, 4, 10); lcd.drawBox(88, y + 9, 4, 10);
  if (t % 3200 < 180) {
    lcd.drawHLine(49, y + 10, 8); lcd.drawHLine(72, y + 10, 8);
  } else {
    int look = (int)roundf(sinf(t * .002f) * 2);
    lcd.drawRFrame(48, y + 5, 11, 12, 3); lcd.drawRFrame(69, y + 5, 11, 12, 3);
    lcd.drawBox(52 + look, y + 9, 3, 5); lcd.drawBox(73 + look, y + 9, 3, 5);
  }
  lcd.drawHLine(57, y + 21, 14);
  lcd.drawPixel(56, y + 20); lcd.drawPixel(71, y + 20);
  lcd.drawRFrame(52, y + 29, 24, 8, 2);
  int wave = (int)roundf(sinf(t * .008f) * 5);
  lcd.drawLine(77, y + 31, 96, y + 22 + wave);
  lcd.drawCircle(98, y + 20 + wave, 2);
  centered("ESP32-S3 + 128x64", 63);
}

void radar(uint32_t t) {
  header();
  for (int r = 8; r <= 24; r += 8) lcd.drawCircle(31, 37, r);
  lcd.drawHLine(7, 37, 49); lcd.drawVLine(31, 13, 49);
  float a = (t % 5000) * TAU / 5000;
  for (int i = 0; i < 3; ++i)
    lcd.drawLine(31, 37, 31 + lroundf(cosf(a - i * .13f) * 24),
                 37 + lroundf(sinf(a - i * .13f) * 24));
  if (t % 1800 < 1300) lcd.drawDisc(43, 23, 2);
  if (t % 2500 < 1700) lcd.drawDisc(19, 47, 2);
  lcd.drawStr(65, 25, "EXPLORANDO"); lcd.drawStr(65, 38, "128 x 64");
  lcd.drawStr(65, 51, "SIMULACAO");
}

void cube(uint32_t t) {
  header();
  int x[8], y[8];
  float a = t * .0009f, b = t * .0006f;
  for (int i = 0; i < 8; ++i) {
    float xx = i & 1 ? 1 : -1, yy = i & 2 ? 1 : -1, zz = i & 4 ? 1 : -1;
    float xr = xx * cosf(a) + zz * sinf(a), zr = -xx * sinf(a) + zz * cosf(a);
    float yr = yy * cosf(b) - zr * sinf(b), z = yy * sinf(b) + zr * cosf(b);
    x[i] = 48 + lroundf(xr * 56 / (4 + z)); y[i] = 37 + lroundf(yr * 56 / (4 + z));
  }
  for (int i = 0; i < 8; ++i)
    for (int bit = 1; bit <= 4; bit <<= 1)
      if (!(i & bit)) lcd.drawLine(x[i], y[i], x[i | bit], y[i | bit]);
  lcd.drawStr(85, 28, "3 EIXOS"); lcd.drawStr(85, 41, "8 PONTOS");
  lcd.drawStr(85, 54, "12 AREST");
}

void stars(uint32_t t) {
  // Trajetorias deterministicas: pausa congela exatamente a imagem.
  for (int i = 0; i < 48; ++i) {
    float a = ((i * 137) % 360) * TAU / 360;
    float z = 1 + ((t / 14 + i * 97) % 800) / 20.0f;
    int x = 64 + lroundf(cosf(a) * z * 1.8f), y = 32 + lroundf(sinf(a) * z);
    if (x >= 1 && x <= 126 && y >= 1 && y <= 62) {
      if (z > 24) lcd.drawBox(x, y, 2, 2); else lcd.drawPixel(x, y);
    }
  }
  lcd.drawLine(59, 35, 64, 25); lcd.drawLine(64, 25, 69, 35);
  lcd.drawHLine(59, 35, 11); lcd.drawVLine(64, 36, 2 + (t / 80) % 4);
}

int triangle(uint32_t t, int range) {
  int v = t % (range * 2);
  return v <= range ? v : range * 2 - v;
}

void pong(uint32_t t) {
  header();
  lcd.drawFrame(0, 12, 128, 52);
  for (int y = 14; y < 62; y += 6) lcd.drawVLine(64, y, 3);
  int bx = 9 + triangle(t / 12, 108), by = 16 + triangle(t / 17, 42);
  int left = constrain(by - 6 + (int)roundf(sinf(t * .003f) * 5), 14, 49);
  int right = constrain(by - 6, 14, 49);
  lcd.drawBox(4, left, 3, 13); lcd.drawBox(121, right, 3, 13);
  lcd.drawBox(bx, by, 3, 3);
  lcd.setFont(u8g2_font_6x10_tr);
  lcd.drawStr(44, 24, "03"); lcd.drawStr(73, 24, "02");
}

void waves(uint32_t t) {
  header();
  for (int x = 0; x < 128; x += 16) lcd.drawVLine(x, 12, 52);
  for (int y = 16; y < 64; y += 12) lcd.drawHLine(0, y, 128);
  int prev = 38;
  for (int x = 0; x < 128; ++x) {
    int y = 38 + lroundf(sinf(x * .10f - t * .004f) * 14 + sinf(x * .23f + t * .002f) * 5);
    if (x) lcd.drawLine(x - 1, prev, x, y);
    prev = y;
  }
}

void equalizer(uint32_t t) {
  header();
  for (int i = 0; i < 16; ++i) {
    int h = 4 + lroundf((sinf(t * .003f + i * .65f) + 1) * 15 +
                       (sinf(t * .007f - i * .8f) + 1) * 5);
    for (int y = 60; y > 60 - h; y -= 4) lcd.drawBox(i * 8 + 1, y - 2, 6, 3);
    lcd.drawHLine(i * 8 + 1, max(13, 55 - h), 6);
  }
}

void clockDemo(uint32_t t) {
  header();
  uint32_t sec = t / 1000;
  float s = (sec % 60) * TAU / 60 - TAU / 4;
  float m = ((sec / 60) % 60 + (sec % 60) / 60.0f) * TAU / 60 - TAU / 4;
  float h = (10 + (sec / 3600.0f)) * TAU / 12 - TAU / 4;
  lcd.drawCircle(29, 37, 24);
  for (int i = 0; i < 12; ++i) {
    float a = i * TAU / 12;
    lcd.drawDisc(29 + lroundf(cosf(a) * 21), 37 + lroundf(sinf(a) * 21), 1);
  }
  lcd.drawLine(29, 37, 29 + lroundf(cosf(s) * 20), 37 + lroundf(sinf(s) * 20));
  lcd.drawLine(29, 37, 29 + lroundf(cosf(m) * 17), 37 + lroundf(sinf(m) * 17));
  lcd.drawLine(29, 37, 29 + lroundf(cosf(h) * 12), 37 + lroundf(sinf(h) * 12));
  char b[16]; snprintf(b, sizeof(b), "10:%02lu:%02lu", (sec / 60) % 60, sec % 60);
  lcd.setFont(u8g2_font_6x10_tr); lcd.drawStr(64, 29, b);
  lcd.setFont(u8g2_font_5x8_tr); lcd.drawStr(64, 43, "HORA FICTICIA");
  lcd.drawStr(64, 57, "SEM INTERNET");
}

void pixels(uint32_t t) {
  // 8 fases de 1,5s: apagado, aceso, xadrez fino, xadrez 4px,
  // linhas verticais, horizontais, borda/diagonais, varredura.
  int phase = (t / 1500) % 8;
  if (phase == 0) return;
  if (phase == 1) { lcd.drawBox(0, 0, 128, 64); return; }
  if (phase == 2 || phase == 3) {
    int step = phase == 2 ? 1 : 4;
    for (int y = 0; y < 64; y += step)
      for (int x = 0; x < 128; x += step)
        if (((x / step + y / step) & 1) == 0) lcd.drawBox(x, y, step, step);
  } else if (phase == 4) {
    for (int x = 0; x < 128; x += 2) lcd.drawVLine(x, 0, 64);
  } else if (phase == 5) {
    for (int y = 0; y < 64; y += 2) lcd.drawHLine(0, y, 128);
  } else {
    lcd.drawFrame(0, 0, 128, 64);
    if (phase == 6) {
      lcd.drawLine(0, 0, 127, 63); lcd.drawLine(0, 63, 127, 0);
    } else {
      lcd.drawVLine((t / 12) % 128, 0, 64);
      lcd.drawHLine(0, (t / 24) % 64, 128);
    }
  }
}

void halves(uint32_t t) {
  int phase = (t / 3000) % 4;
  if (phase == 0) lcd.drawBox(0, 0, 64, 64);
  if (phase == 1) lcd.drawBox(64, 0, 64, 64);
  if (phase < 2) {
    lcd.setDrawColor(0); lcd.setFont(u8g2_font_6x10_tr);
    lcd.drawStr(phase * 64 + 6, 29, phase == 0 ? "ESQUERDA" : "DIREITA");
    const bool physicalCs2 = (phase == 0) == swapHalves;
    lcd.drawStr(phase * 64 + 22, 43, physicalCs2 ? "CS2" : "CS1");
    lcd.setDrawColor(1);
  } else {
    lcd.drawFrame(0, 0, 128, 64); lcd.drawVLine(63, 0, 64);
    lcd.setFont(u8g2_font_6x10_tr);
    lcd.drawStr(9, 24, swapHalves ? "CS2" : "CS1");
    lcd.drawStr(82, 24, swapHalves ? "CS1" : "CS2");
    for (int y = 0; y < 64; y += 8) {
      lcd.drawBox(2, y + 2, 3, 3); lcd.drawBox(123, y + 2, 3, 3);
    }
    lcd.drawHLine(3, 42, 122);
    lcd.drawDisc(5 + triangle(t / 20, 117), 42, 4);
    lcd.setFont(u8g2_font_5x8_tr); centered("CRUZANDO AS METADES", 59);
  }
}

void status() {
  Serial.printf("STATUS build=lcm12864-demo-v4 scene=%u auto=%u paused=%u invert=%u rotate=%u csLow=%u swap=%u frames=%lu transfer_us=%lu hash=%08lx uptime_ms=%lu heap=%u\n",
      scene + 1, automatic, paused, inverted, rotated, csActiveLow, swapHalves,
      frameCount, transferUs, bufferHash, millis(), ESP.getFreeHeap());
}

void help() {
  Serial.println("\nDEMO LCM12864D-V1.0 WHITE / KS0108 / 128x64 / v4");
  Serial.println("VDD LCD=5V. CS ativo LOW. Pot 20k entre VDD/VEE, centro em VO.");
  Serial.println("Ordem visual: esquerda=CS1/GPIO12, direita=CS2/GPIO13.");
  Serial.println("DB0..7=GPIO4..11 CS1=12 CS2=13 RST=14 RS=15 E=16 RW=GND");
  Serial.println("BOOT curto=proxima cena; longo=pausa. Ciclo automatico: 12s/cena.");
  Serial.println("Serial 115200: 1..9=cena fixa, 0=cena 10, n=proxima, a=auto, p=pausa");
  Serial.println("i=inverter pixels, r=girar 180, c=polaridade CS, w=trocar metades");
  Serial.println("d=reinicializar LCD, s=status, b=buffer hex, h=ajuda. Ajustes nao persistem.");
  Serial.println("Contraste: potenciometro. RW em GND: nao ha confirmacao de resposta do LCD.");
  for (int i = 0; i < SCENES; ++i) Serial.printf("%u: %s\n", i + 1, names[i]);
}

void selectScene(uint8_t next) {
  scene = next % SCENES; animationMs = sceneElapsed = 0;
  Serial.printf("SCENE %u %s\n", scene + 1, names[scene]);
}

void initLcd() {
  digitalWrite(ENABLE_LCD, LOW);
  lcd.begin();
  digitalWrite(CS_LEFT, csActiveLow ? HIGH : LOW);
  digitalWrite(CS_RIGHT, csActiveLow ? HIGH : LOW);
  lcd.setDisplayRotation(rotated ? U8G2_R2 : U8G2_R0);
}

void serialInput() {
  for (int count = 0; count < 16 && Serial.available(); ++count) {
    char c = Serial.read();
    if (c >= '0' && c <= '9') {
      automatic = false; selectScene(c == '0' ? 9 : c - '1');
    } else if (c == 'n') selectScene(scene + 1);
    else if (c == 'a') { automatic = !automatic; sceneElapsed = 0; }
    else if (c == 'p') paused = !paused;
    else if (c == 'i') inverted = !inverted;
    else if (c == 'r') { rotated = !rotated; lcd.setDisplayRotation(rotated ? U8G2_R2 : U8G2_R0); }
    else if (c == 'c' || c == 'w' || c == 'd') {
      if (c == 'c') csActiveLow = !csActiveLow;
      if (c == 'w') swapHalves = !swapHalves;
      initLcd();
    } else if (c == 'b') {
      Serial.print("BUFFER ");
      for (size_t i = 0; i < 1024; ++i) Serial.printf("%02x", lcd.getBufferPtr()[i]);
      Serial.println();
    } else if (c == 'h' || c == '?') help();
    if (c != '\n' && c != '\r') status();
  }
}

void buttonInput(uint32_t now) {
  static bool last = HIGH, stable = HIGH, longHandled = false;
  static uint32_t changedAt = 0, pressedAt = 0;
  bool reading = digitalRead(BUTTON);
  if (reading != last) { last = reading; changedAt = now; }
  if (reading != stable && now - changedAt >= 30) {
    stable = reading;
    if (!stable) { pressedAt = now; longHandled = false; }
    else if (!longHandled) { selectScene(scene + 1); status(); }
  }
  if (!stable && !longHandled && now - pressedAt >= 800) {
    paused = !paused; longHandled = true; status();
  }
}

void render() {
  lcd.clearBuffer(); lcd.setDrawColor(1); lcd.setFont(u8g2_font_5x8_tr);
  switch (scene) {
    case 0: robot(animationMs); break; case 1: radar(animationMs); break;
    case 2: cube(animationMs); break; case 3: stars(animationMs); break;
    case 4: pong(animationMs); break; case 5: waves(animationMs); break;
    case 6: equalizer(animationMs); break; case 7: clockDemo(animationMs); break;
    case 8: pixels(animationMs); break; case 9: halves(animationMs); break;
  }
  uint8_t *buf = lcd.getBufferPtr();
  bufferHash = 2166136261UL;
  for (size_t i = 0; i < 1024; ++i) {
    if (inverted) buf[i] ^= 0xFF;
    bufferHash = (bufferHash ^ buf[i]) * 16777619UL;
  }
  uint32_t started = micros(); lcd.sendBuffer(); transferUs = micros() - started;
  ++frameCount;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  for (int pin = 4; pin <= 16; ++pin) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
  digitalWrite(RESET_LCD, HIGH);
  baseGpio = lcd.getU8x8()->gpio_and_delay_cb;
  lcd.getU8x8()->gpio_and_delay_cb = lcdGpio;
  delay(300); initLcd(); help();
  Serial.printf("BOOT reset_reason=%d flash=%u psram=%u\n", esp_reset_reason(), ESP.getFlashChipSize(), ESP.getPsramSize());
  selectScene(0); render(); lastTick = lastFrame = millis();
}

void loop() {
  uint32_t now = millis();
  if (!paused) { animationMs += now - lastTick; sceneElapsed += now - lastTick; }
  lastTick = now;
  serialInput(); buttonInput(now);
  if (automatic && !paused && sceneElapsed >= SCENE_MS) selectScene(scene + 1);
  if (now - lastFrame >= FRAME_MS) { lastFrame = now; render(); }
  delay(1);
}

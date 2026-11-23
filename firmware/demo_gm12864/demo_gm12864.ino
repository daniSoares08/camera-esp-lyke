#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>

// GM12864-59N VER 2.0 IIC / ST7567S, quatro fios.
// GPIO41 e GPIO42 ficam livres da camera e da USB nesta placa.
constexpr uint8_t LCD_SDA = 41;
constexpr uint8_t LCD_SCL = 42;
constexpr uint8_t BUTTON = 0;  // Botao BOOT da placa, ativo em LOW.
constexpr uint32_t I2C_HZ = 100000;
constexpr uint32_t FRAME_MS = 120;
constexpr uint32_t SCENE_MS = 10000;
constexpr float PI_F = 3.14159265f;

U8G2_ST7567_ENH_DG128064I_F_HW_I2C lcd(
    U8G2_R0, U8X8_PIN_NONE, LCD_SCL, LCD_SDA);

const char *const sceneNames[] = {"ROBO", "RADAR", "CUBO 3D", "PIXELS"};
uint8_t scene = 0, address = 0, contrast = 200;
bool ready = false, automatic = true, paused = false, inverted = false;
bool rotated = false;
uint32_t sceneStart = 0, lastFrame = 0, lastProbe = 0;
uint32_t animationMs = 0, lastTick = 0;

void printHelp() {
  Serial.println("\nDEMO GM12864-59N / ST7567S 128x64");
  Serial.printf("Ligacao: VCC=3V3 GND=GND SDA=%u SCL=%u\n", LCD_SDA, LCD_SCL);
  Serial.println("BOOT curto: proxima cena. BOOT longo: pausa animacao.");
  Serial.println("Serial: n=proxima a=automatico p=pausa 1..4=cena fixa");
  Serial.println("        +=contraste -=contraste r=girar180 i=inverter s=scan h=ajuda");
  Serial.println("Monitor USB OTG: 115200. Ajustes voltam ao padrao ao reiniciar.");
}

bool responds(uint8_t candidate) {
  Wire.beginTransmission(candidate);
  return Wire.endTransmission() == 0;
}

void scanBus() {
  uint8_t count = 0;
  Serial.println("SCAN I2C (enderecos de 7 bits):");
  for (uint8_t a = 8; a < 120; ++a) {
    if (responds(a)) {
      Serial.printf("  0x%02X%s\n", a, a == 0x3F || a == 0x3E ? " candidato ST7567S" : "");
      ++count;
    }
  }
  if (!count) Serial.println("  Nenhum dispositivo: confira 3V3, GND, SDA e SCL.");
}

void configureDisplay() {
  lcd.setContrast(contrast);
  lcd.setDisplayRotation(rotated ? U8G2_R2 : U8G2_R0);
  lcd.sendF("c", inverted ? 0xA7 : 0xA6);
}

bool connectDisplay() {
  address = responds(0x3F) ? 0x3F : (responds(0x3E) ? 0x3E : 0);
  if (!address) {
    Serial.println("LCD ausente em 0x3F/0x3E; nova tentativa em 3s. Use s para scan.");
    return false;
  }
  // U8g2 recebe endereco de 8 bits; Wire e o scan usam 7 bits.
  lcd.setI2CAddress(address << 1);
  lcd.setBusClock(I2C_HZ);
  lcd.begin();
  Wire.setTimeOut(50);
  configureDisplay();
  Serial.printf("LCD respondeu em 0x%02X; contraste=%u. Confira a imagem na tela.\n", address, contrast);
  return true;
}

void centered(const char *text, uint8_t baseline) {
  const int width = lcd.getStrWidth(text);
  lcd.drawStr(width < 128 ? (128 - width) / 2 : 0, baseline, text);
}

void header(const char *title) {
  lcd.setFont(u8g2_font_5x8_tr);
  lcd.drawStr(3, 8, title);
  char status[8];
  snprintf(status, sizeof(status), "%u/4 %c", scene + 1,
           paused ? 'P' : (automatic ? 'A' : 'M'));
  lcd.drawStr(96, 8, status);
  lcd.drawHLine(0, 10, 128);
}

void drawRobot(uint32_t t) {
  header("OLA, HUMANO!");
  const int bob = (int)roundf(sinf(t * 0.004f) * 2);
  const int y = 18 + bob;
  const bool blink = t % 3200 < 180;
  lcd.drawRFrame(40, y, 48, 27, 5);
  lcd.drawLine(64, y, 64, y - 4);
  lcd.drawDisc(64, y - 6, 2);
  lcd.drawBox(36, y + 9, 4, 10);
  lcd.drawBox(88, y + 9, 4, 10);
  if (blink) {
    lcd.drawHLine(49, y + 10, 8);
    lcd.drawHLine(72, y + 10, 8);
  } else {
    const int look = (int)roundf(sinf(t * 0.002f) * 2);
    lcd.drawRFrame(48, y + 5, 11, 12, 3);
    lcd.drawRFrame(69, y + 5, 11, 12, 3);
    lcd.drawBox(52 + look, y + 9, 3, 5);
    lcd.drawBox(73 + look, y + 9, 3, 5);
  }
  lcd.drawHLine(57, y + 21, 14);
  lcd.drawPixel(56, y + 20);
  lcd.drawPixel(71, y + 20);
  lcd.drawRFrame(52, y + 29, 24, 9, 2);
  lcd.drawBox(58, y + 32, 3, 3);
  lcd.drawBox(67, y + 32, 3, 3);
  // Braco acena; particulas mudam de posicao junto da animacao.
  const int wave = (int)roundf(sinf(t * 0.008f) * 5);
  lcd.drawLine(77, y + 31, 96, y + 23 + wave);
  lcd.drawCircle(98, y + 21 + wave, 2);
  for (uint8_t i = 0; i < 8; ++i) {
    const int x = 5 + (i * 17 + t / 110) % 118;
    const int sy = 15 + (i * 13) % 33;
    if (x < 30 || x > 105) lcd.drawPixel(x, sy);
  }
  lcd.setFont(u8g2_font_5x8_tr);
  centered("ESP32-S3 + LCD = OK", 63);
}

void drawRadar(uint32_t t) {
  header("RADAR / DEMO");
  constexpr int cx = 32, cy = 37, radius = 24;
  lcd.drawCircle(cx, cy, radius);
  lcd.drawCircle(cx, cy, 16);
  lcd.drawCircle(cx, cy, 8);
  lcd.drawHLine(cx - radius, cy, radius * 2 + 1);
  lcd.drawVLine(cx, cy - radius, radius * 2 + 1);
  const float angle = (t % 5000) * (2.0f * PI_F / 5000.0f);
  for (uint8_t i = 0; i < 3; ++i) {
    const float a = angle - i * 0.12f;
    lcd.drawLine(cx, cy, cx + (int)roundf(cosf(a) * radius),
                 cy + (int)roundf(sinf(a) * radius));
  }
  if (t % 1500 < 1000) lcd.drawDisc(45, 26, 2);
  if (t % 2200 < 1500) lcd.drawDisc(20, 48, 2);
  lcd.drawStr(65, 23, "EXPLORANDO");
  lcd.drawStr(65, 35, "128 x 64");
  lcd.drawStr(65, 47, "I2C 100kHz");
  lcd.drawStr(65, 59, "SIMULACAO");
}

void drawCube(uint32_t t) {
  header("CUBO 3D");
  int px[8], py[8];
  const float a = t * 0.0009f, b = t * 0.0006f;
  const float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
  for (uint8_t i = 0; i < 8; ++i) {
    const float x = (i & 1) ? 1.0f : -1.0f;
    const float y = (i & 2) ? 1.0f : -1.0f;
    const float z = (i & 4) ? 1.0f : -1.0f;
    const float xr = x * ca + z * sa, zr = -x * sa + z * ca;
    const float yr = y * cb - zr * sb, zz = y * sb + zr * cb;
    const float scale = 52.0f / (4.0f + zz);
    px[i] = 44 + (int)roundf(xr * scale);
    py[i] = 37 + (int)roundf(yr * scale);
  }
  for (uint8_t i = 0; i < 8; ++i) {
    for (uint8_t bit = 1; bit <= 4; bit <<= 1) {
      if (!(i & bit)) lcd.drawLine(px[i], py[i], px[i | bit], py[i | bit]);
    }
  }
  lcd.drawStr(79, 27, "SEM WIFI");
  lcd.drawStr(79, 39, "SEM APP");
  lcd.drawStr(79, 51, "SO PIXEL");
  centered("BOOT: trocar cena", 63);
}

void drawPixels(uint32_t t) {
  // Cinco padroes: borda/diagonais, xadrez, linhas, tudo aceso, tudo apagado.
  const uint8_t phase = (t / 2000) % 5;
  if (phase == 3) {
    lcd.drawBox(0, 0, 128, 64);
    return;
  }
  if (phase == 4) return;
  lcd.drawFrame(0, 0, 128, 64);
  if (phase == 0) {
    lcd.drawLine(0, 0, 127, 63);
    lcd.drawLine(0, 63, 127, 0);
  } else if (phase == 1) {
    for (uint8_t y = 0; y < 64; y += 4)
      for (uint8_t x = 0; x < 128; x += 4)
        if (((x / 4) + (y / 4)) % 2 == 0) lcd.drawBox(x, y, 4, 4);
  } else {
    for (uint8_t x = 2; x < 126; x += 4) lcd.drawVLine(x, 1, 62);
  }
  lcd.setDrawColor(0);
  lcd.drawBox(26, 23, 76, 19);
  lcd.setDrawColor(1);
  lcd.setFont(u8g2_font_6x10_tr);
  centered("TESTE PIXELS", 35);
}

void selectScene(uint8_t selected) {
  scene = selected % 4;
  sceneStart = millis();
  animationMs = 0;
  Serial.printf("Cena %u: %s / automatico=%u pausa=%u\n", scene + 1,
                sceneNames[scene], automatic, paused);
}

void handleSerial() {
  // Limite por passagem evita que muito texto no monitor trave o botao.
  for (uint8_t processed = 0; processed < 16 && Serial.available(); ++processed) {
    const char c = Serial.read();
    if (c == 'n') selectScene(scene + 1);
    else if (c >= '1' && c <= '4') {
      automatic = false;
      selectScene(c - '1');
    } else if (c == 'a') {
      automatic = !automatic;
      sceneStart = millis();
      Serial.printf("Automatico=%u\n", automatic);
    } else if (c == 'p') {
      paused = !paused;
      sceneStart = millis();
      Serial.printf("Pausa=%u\n", paused);
    } else if (c == '+' || c == '-') {
      contrast = (uint8_t)constrain((int)contrast + (c == '+' ? 4 : -4), 0, 252);
      if (ready) lcd.setContrast(contrast);
      Serial.printf("Contraste=%u\n", contrast);
    } else if (c == 'r' || c == 'i') {
      if (c == 'r') rotated = !rotated;
      else inverted = !inverted;
      if (ready) configureDisplay();
      Serial.printf("Girar180=%u inverter=%u\n", rotated, inverted);
    } else if (c == 's') scanBus();
    else if (c == 'h' || c == '?') printHelp();
  }
}

void handleButton(uint32_t now) {
  static bool lastReading = HIGH, stable = HIGH, longHandled = false;
  static uint32_t changedAt = 0, pressedAt = 0;
  const bool reading = digitalRead(BUTTON);
  if (reading != lastReading) {
    lastReading = reading;
    changedAt = now;
  }
  if (reading != stable && now - changedAt >= 30) {
    stable = reading;
    if (stable == LOW) {
      pressedAt = now;
      longHandled = false;
    } else if (!longHandled) {
      selectScene(scene + 1);
    }
  }
  if (stable == LOW && !longHandled && now - pressedAt >= 800) {
    paused = !paused;
    sceneStart = now;
    longHandled = true;
    Serial.printf("Pausa=%u\n", paused);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  delay(300);  // Nao espera pela conexao de um computador.
  printHelp();
  Wire.begin(LCD_SDA, LCD_SCL, I2C_HZ);
  Wire.setTimeOut(50);
  ready = connectDisplay();
  lastProbe = sceneStart = lastTick = millis();
}

void loop() {
  handleSerial();
  const uint32_t now = millis();
  handleButton(now);
  if (!paused) animationMs += now - lastTick;
  lastTick = now;
  if (now - lastProbe >= 3000) {
    lastProbe = now;
    if (ready && !responds(address)) {
      ready = false;
      Serial.println("LCD parou de responder. Desligue a alimentacao antes de refazer os fios.");
    }
    if (!ready) ready = connectDisplay();
  }
  if (automatic && !paused && now - sceneStart >= SCENE_MS) selectScene(scene + 1);
  if (ready && now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    lcd.clearBuffer();
    lcd.setDrawColor(1);
    lcd.setFont(u8g2_font_5x8_tr);
    switch (scene) {
      case 0: drawRobot(animationMs); break;
      case 1: drawRadar(animationMs); break;
      case 2: drawCube(animationMs); break;
      case 3: drawPixels(animationMs); break;
    }
    lcd.sendBuffer();
  }
  delay(1);
}

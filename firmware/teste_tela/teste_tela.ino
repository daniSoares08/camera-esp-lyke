#include <Arduino.h>
#include <U8g2lib.h>

// Pinagem soldada no ESP32-S3: nao alterar.
#define LCD_SCL 12
#define LCD_SI  11
#define LCD_CS  10
#define LCD_RS   9
#define LCD_RSE  8

U8G2_ST7565_ERC12864_ALT_F_4W_SW_SPI u8g2(
    U8G2_R0, LCD_SCL, LCD_SI, LCD_CS, LCD_RS, LCD_RSE);

void desenharTela() {
    char tempo[24];
    snprintf(tempo, sizeof(tempo), "Ligado: %lu s", millis() / 1000UL);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(5, 15, "ESP32-S3");
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.drawStr(5, 37, "TELA OK!");
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawLine(5, 45, 123, 45);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 58, tempo);
    if ((millis() / 1000UL) % 2 == 0) u8g2.drawBox(116, 51, 6, 6);
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    u8g2.setContrast(75);
    desenharTela();
    Serial.println("Display inicializado! SCL=12 SI=11 CS=10 RS=9 RESET=8 contraste=75");
}

void loop() {
    static unsigned long anterior = 0;
    if (millis() - anterior >= 1000) {
        anterior = millis();
        desenharTela();
        Serial.printf("TESTE_TELA OK uptime=%lu s contraste=75\n", millis() / 1000UL);
    }
    delay(10);
}

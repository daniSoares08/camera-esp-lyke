#pragma once
#include "image_format.h"

// The web upload is validated in full before resetting the STM32. The STM32
// separately stages, verifies and commits it before modifying its application.
class StmUpdate {
  static constexpr size_t Limit = LYKE_PACK_LIMIT;
  uint8_t* image = nullptr;
  size_t bytes = 0;
  bool accepted = false;

  bool waitFor(const char* expected, uint32_t timeout) {
    char text[64];
    String lastReceived;
    size_t used = 0;
    bool overlong = false;
    const uint32_t start = millis();
    while (millis() - start < timeout) {
      if (!stmSerial.available()) { delay(1); continue; }
      int c = stmSerial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        text[used] = 0;
        if (!overlong && used) lastReceived = text;
        if (!overlong && strcmp(text, expected) == 0) return true;
        if (!overlong && strcmp(text, "LYKE BOOT")==0) {
          error="Carregador STM32 antigo. Instale a base R10 uma vez por USB.";
          return false;
        }
        if (!overlong && (strstr(text, "_ERROR") || strstr(text, "BAD_HEADER"))) {
          error = text;
          return false;
        }
        used = 0;
        overlong = false;
      } else if (used + 1 < sizeof(text)) {
        text[used++] = (char)c;
      } else {
        overlong = true;
      }
    }
    error = String("Tempo esgotado: ") + expected;
    if (lastReceived.length()) error += "; RX=" + lastReceived;
    else error += "; sem bytes UART";
    return false;
  }
  bool install(uint32_t size, uint32_t crc) {
    while (stmSerial.available()) stmSerial.read();
    // The application command also works while mapping.
    stmSerial.print("LYKE-BOOT-V1\n");
    if (!waitFor("LYKE BOOT V2", 5000)) return false;
    stmSerial.printf("FLASH %08lx %08lx\n", (unsigned long)size, (unsigned long)crc);
    if (!waitFor("LYKE READY_DATA", 5000)) return false;
    for (size_t offset = 0; offset < size; offset += 256) {
      const size_t n = min(size_t(256), size_t(size) - offset);
      if (stmSerial.write(image + offset, n) != n || !waitFor("LYKE ACK", 2800))
        return false;
    }
    if (!waitFor("LYKE INSTALLED", 20000)) return false;
    if (!waitFor("LYKE APP V1", 4000)) return false;
    return true;
  }
public:
  String error;
  void reset() {
    free(image);
    image = nullptr;
    accepted = false;
    bytes = 0;
  }
  void start(bool allowed, const String& name) {
    reset();
    error = "";
    if (!allowed || name != "lyke-stm32-ota.bin") {
      error = "Arquivo esperado: lyke-stm32-ota.bin";
      return;
    }
    if (ESP.getFreePsram() < Limit + LYKE_PACK_BLOCK + 512 * 1024) {
      error = "PSRAM insuficiente";
      return;
    }
    image = static_cast<uint8_t*>(heap_caps_malloc(Limit + LYKE_PACK_BLOCK, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!image) error = "Sem memoria para imagem STM32";
    accepted = image;
  }
  void write(const uint8_t* p, size_t n) {
    if (!accepted) return;
    if (n > Limit - bytes) {
      reset();
      error = "Imagem STM32 grande demais";
      return;
    }
    memcpy(image + bytes, p, n);
    bytes += n;
  }
  bool finish() {
    if (!accepted) return false;
    if (bytes < LYKE_PACK_HEADER + 4) {
      error = "Imagem STM32 incompleta";
      reset();
      return false;
    }
    if (!image_validate(image, bytes, image+Limit)) {
      error = "Pacote STM32 v2 invalido ou incompleto";
      reset();
      return false;
    }
    bool ok = install(bytes, ~image_crc_update(UINT32_MAX, image, bytes));
    reset();
    return ok;
  }
} stmUpdate;

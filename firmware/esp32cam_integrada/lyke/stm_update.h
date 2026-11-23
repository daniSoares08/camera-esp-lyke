#pragma once

// The web upload is validated in full before resetting the STM32. The STM32
// separately stages, verifies and commits it before modifying its application.
class StmUpdate {
  static constexpr size_t Limit = 56 * 1024;
  static constexpr size_t Header = 16;
  uint8_t* image = nullptr;
  size_t bytes = 0;
  bool accepted = false;

  static uint32_t word(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
  }
  static uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t crc = UINT32_MAX;
    while (n--) {
      crc ^= *p++;
      for (unsigned bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
  }
  bool waitFor(const char* expected, uint32_t timeout) {
    char text[64];
    size_t used = 0;
    bool overlong = false;
    const uint32_t start = millis();
    while (millis() - start < timeout) {
      if (!stmSerial.available()) { delay(1); continue; }
      int c = stmSerial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        text[used] = 0;
        if (!overlong && strcmp(text, expected) == 0) return true;
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
    return false;
  }
  bool install(uint32_t size, uint32_t crc) {
    while (stmSerial.available()) stmSerial.read();
    // The application command also works while mapping.
    stmSerial.print("LYKE-BOOT-V1\n");
    if (!waitFor("LYKE BOOT", 3200)) return false;
    stmSerial.printf("FLASH %08lx %08lx\n", (unsigned long)size, (unsigned long)crc);
    if (!waitFor("LYKE READY_DATA", 5000)) return false;
    for (size_t offset = 0; offset < size; offset += 256) {
      const size_t n = min(size_t(256), size_t(size) - offset);
      if (stmSerial.write(image + Header + offset, n) != n || !waitFor("LYKE ACK", 2800))
        return false;
    }
    if (!waitFor("LYKE INSTALLED", 8500)) return false;
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
    if (ESP.getFreePsram() < Limit + Header + 512 * 1024) {
      error = "PSRAM insuficiente";
      return;
    }
    image = static_cast<uint8_t*>(heap_caps_malloc(Limit + Header, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!image) error = "Sem memoria para imagem STM32";
    accepted = image;
  }
  void write(const uint8_t* p, size_t n) {
    if (!accepted) return;
    if (n > Limit + Header - bytes) {
      reset();
      error = "Imagem STM32 grande demais";
      return;
    }
    memcpy(image + bytes, p, n);
    bytes += n;
  }
  bool finish() {
    if (!accepted) return false;
    if (bytes < Header + 512) {
      error = "Imagem STM32 incompleta";
      reset();
      return false;
    }
    const uint32_t size = word(image + 8), crc = word(image + 12);
    if (memcmp(image, "LYKEG431", 8) ||
        size > Limit || size != bytes - Header || word(image + Header) != 0x200057f0 ||
        !(word(image + Header + 4) & 1) || word(image + Header + 4) < 0x08002009 ||
        word(image + Header + 4) - 0x08002000 >= size || crc32(image + Header, size) != crc) {
      error = "Imagem STM32 invalida ou incompleta";
      reset();
      return false;
    }
    bool ok = install(size, crc);
    reset();
    return ok;
  }
} stmUpdate;

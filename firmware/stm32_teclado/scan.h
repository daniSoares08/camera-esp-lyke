#ifndef STM32_TECLADO_SCAN_H
#define STM32_TECLADO_SCAN_H

#include <stdbool.h>
#include <stdint.h>

enum { TRACK_COUNT = 19, MAX_PAIRS = TRACK_COUNT * (TRACK_COUNT - 1) / 2 };

typedef enum { IDLE, CONTACT, MULTIPLE, LOW_AT_REST } ContactKind;

typedef struct {
    ContactKind kind;
    uint8_t first;
    uint8_t second;
} Contact;

typedef struct {
    uint8_t first;
    uint8_t second;
} Pair;

typedef struct {
    Contact pending;
    Contact stable;
    uint32_t changed_at;
    bool armed;
    bool initialized;
    uint16_t count;
    uint16_t last_number;
    bool duplicate;
    Pair log[MAX_PAIRS];
} Mapper;

static inline bool contact_equal(Contact a, Contact b)
{
    return a.kind == b.kind && a.first == b.first && a.second == b.second;
}

static inline Contact decode_contacts(const uint32_t hits[TRACK_COUNT])
{
    Contact result = { IDLE, 0, 0 };
    for (uint8_t a = 0; a < TRACK_COUNT; ++a) {
        for (uint8_t b = a + 1; b < TRACK_COUNT; ++b) {
            if ((hits[a] & (1u << b)) == 0 && (hits[b] & (1u << a)) == 0)
                continue;
            if (result.kind == CONTACT)
                return (Contact){ MULTIPLE, 0, 0 };
            result = (Contact){ CONTACT, a, b };
        }
    }
    return result;
}

static inline bool map_update(Mapper *map, Contact reading, uint32_t now)
{
    if (!contact_equal(reading, map->pending)) {
        map->pending = reading;
        map->changed_at = now;
    }
    if (now - map->changed_at < 35 ||
        (map->initialized && contact_equal(reading, map->stable)))
        return false;

    map->initialized = true;
    map->stable = reading;
    if (reading.kind == IDLE) {
        map->armed = true;
        return true;
    }

    bool accept = map->armed && reading.kind == CONTACT;
    map->armed = false;
    if (!accept)
        return true;

    for (uint16_t i = 0; i < map->count; ++i) {
        Pair pair = map->log[i];
        if (pair.first == reading.first && pair.second == reading.second) {
            map->last_number = i + 1;
            map->duplicate = true;
            return true;
        }
    }

    if (map->count < MAX_PAIRS) {
        map->log[map->count++] = (Pair){ reading.first, reading.second };
        map->last_number = map->count;
        map->duplicate = false;
    }
    return true;
}

#endif

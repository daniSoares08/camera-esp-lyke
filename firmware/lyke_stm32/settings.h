#pragma once

// Both pages store the same record format. Compaction writes the other page
// before the page holding the current record may be erased.
enum { PROGRAM_CAP = 96, PREF_A = 0x0801f000u, PREF_B = 0x0801f800u };
typedef struct {
    uint32_t revision, packed;
    uint8_t program[PROGRAM_CAP];
    uint32_t checksum, valid;
} Preferences;
_Static_assert(sizeof(Preferences) % 8 == 0, "64-bit flash writes");
static Preferences preferences;
static uint32_t current_slot;
static uint8_t recording_keys[PROGRAM_CAP];
static unsigned recorded_keys;
static bool recording, playing;

static uint32_t preference_checksum(const Preferences *entry)
{
    const uint8_t *p = (const uint8_t *)entry;
    uint32_t hash = 2166136261u;
    for (unsigned i = 0; i < sizeof(*entry) - 8; ++i) hash = (hash ^ p[i]) * 16777619u;
    return hash;
}

static void read_preferences(void)
{
    for (uint32_t page = PREF_A; page <= PREF_B; page += LYKE_PAGE) {
        for (uint32_t addr = page; addr + sizeof(preferences) <= page + LYKE_PAGE; addr += sizeof(preferences)) {
            const Preferences *p = (const Preferences *)addr;
            if (p->valid == 0x31524650u && p->revision > preferences.revision &&
                p->checksum == preference_checksum(p)) {
                preferences = *p;
                current_slot = addr;
            }
        }
    }
}

static bool persist_preferences(uint32_t packed, const uint8_t *keys)
{
    if (preferences.revision && packed == preferences.packed && memcmp(keys, preferences.program, PROGRAM_CAP) == 0)
        return true;
    Preferences next = { .revision = preferences.revision + 1, .packed = packed, .valid = 0x31524650u };
    memcpy(next.program, keys, PROGRAM_CAP);
    next.checksum = preference_checksum(&next);
    uint32_t page = current_slot >= PREF_B ? PREF_B : PREF_A;
    uint32_t addr = current_slot ? current_slot + sizeof(next) : page;
    for (; addr + sizeof(next) <= page + LYKE_PAGE; addr += sizeof(next)) {
        const uint32_t *p = (const uint32_t *)addr;
        unsigned i = 0;
        while (i < sizeof(next)/4 && p[i] == UINT32_MAX) ++i;
        if (i == sizeof(next)/4) break;
    }
    bool rollover = addr + sizeof(next) > page + LYKE_PAGE;
    if (rollover) addr = page == PREF_A ? PREF_B : PREF_A;
    uint32_t words[sizeof(next)/4];
    memcpy(words, &next, sizeof(words));
    flash_begin();
    bool ok = !rollover || flash_erase(addr);
    for (unsigned i = 0; ok && i < sizeof(words)/4; i += 2)
        ok = flash_write(addr + i * 4, words[i], words[i+1]);
    flash_end();
    if (ok) { preferences = next; current_slot = addr; }
    return ok;
}

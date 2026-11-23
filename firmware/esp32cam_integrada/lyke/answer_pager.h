#pragma once
#include <stddef.h>
#include <string.h>

// Pure pager shared by the device and native regression tests. READ/READ4
// retain their old spacing. READ6 compacts old saved answers as well as new ones.
static size_t answer_content_length(const char *text, size_t length)
{
    const char *footer = strstr(text, "\n\nTokens entrada: ");
    if (footer) {
        const char *at = footer + strlen("\n\nTokens entrada: ");
        while (*at >= '0' && *at <= '9') ++at;
        const char *next = "\nTokens saida: ";
        if (strncmp(at, next, strlen(next)) == 0) {
            at += strlen(next);
            while (*at >= '0' && *at <= '9') ++at;
            if (!*at) return (size_t)(footer - text);
        }
    }
    return length;
}

static bool answer_page_rows(const char *text, size_t length, unsigned page,
                             unsigned columns, unsigned rows, bool compact,
                             char output[6][22])
{
    memset(output, 0, 6 * 22);
    if (!columns || columns > 21 || !rows || rows > 6 || page > 999) return false;
    if (compact) length = answer_content_length(text, length);
    size_t start = 0;
    unsigned line = 0, first = page * rows, limit = first + rows;
    while (start < length) {
        if (compact) {
            while (start < length && (text[start] == '\n' || text[start] == '\r' ||
                   text[start] == ' ' || text[start] == '\t')) ++start;
        }
        if (start >= length) break;
        // Look ahead beyond the page only after skipping empty lines/metadata.
        if (line == limit) return true;
        size_t end = start, last_space = start;
        while (end < length && text[end] != '\n' && text[end] != '\r' && end - start < columns) {
            if (text[end] == ' ') last_space = end;
            ++end;
        }
        if (end < length && end - start == columns && text[end] != ' ' &&
            text[end] != '\n' && text[end] != '\r' && last_space > start) end = last_space;
        if (line >= first) {
            size_t used = end - start;
            while (used && text[start + used - 1] == ' ') --used;
            memcpy(output[line - first], text + start, used);
        }
        ++line;
        start = end;
        while (start < length && text[start] == ' ') ++start;
        if (start < length && text[start] == '\r') ++start;
        if (start < length && text[start] == '\n') ++start;
    }
    return false;
}

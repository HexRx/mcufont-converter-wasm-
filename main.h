#pragma once

#include <stdint.h>
#include <sstream>

typedef struct {
    uint8_t *buffer;
    uint32_t width;
    uint32_t height;
    uint32_t y;
    const struct mf_font_s *font;
} state_t;

struct array_result {
  uint32_t size;
  void *data_ptr;
} __attribute__((packed));

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

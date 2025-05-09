#include <stdint.h>
#include <malloc.h>
#include <map>

std::map<uint32_t, uint8_t *> memory;

void write_byte(uint32_t b, uint8_t data) {
  auto page_addr = b & ~0xfff;
  auto found = memory.find(page_addr);
  uint8_t *page_data;
  if (found != memory.end()) {
    page_data = found->second;
  } else {
    page_data = (uint8_t *)calloc(1, 0x1000);
    memory.insert(std::make_pair(page_addr, page_data));
  }
  page_data[b & 0xfff] = data;
}

uint32_t read_byte(uint32_t b) {
  auto page_addr = b & ~0xfff;
  auto found = memory.find(page_addr);
  if (found != memory.end()) {
    return found->second[b & 0xfff];
  }
  return 0;
}

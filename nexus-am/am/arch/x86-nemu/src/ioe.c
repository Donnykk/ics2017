#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  unsigned long ms = inl(RTC_PORT)-boot_time;
  return ms;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  /*
  int i;
  for (i = 0; i < _screen.width * _screen.height; i++) {
    fb[i] = i;
  }
  */
  int temp = (w > _screen.width - x) ? _screen.width : w;
  for(int i=0; i<h && y + i < _screen.height; i++) {
    memcpy(&fb[(y + i) * _screen.width + x], pixels, sizeof(uint32_t) *temp);
    pixels += w;
  }
}

void _draw_sync() {
}

int _read_key() {
  if(inb(0x64))
    return inl(0x60);
  else 
    return _KEY_NONE;
}

void getScreen(int* width, int* height) {
  *width = _screen.width;
  *height = _screen.height; 
}

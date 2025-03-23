#include <Arduino.h>

struct Data
{
    // firstread status
    bool first_read = true;

    int freq = 1000; // Начальная частота 1 кГц
    float VPP = 3.3; // Амплитуда (1.0, 2.5, 3.3)
    int duty = 50;   // Начальная скважность 50%
};

// Меню
enum MenuItem
{
    // VPP,
    STATUS_ITEM,
    FREQ_ITEM,
    HZ_ITEM,
    // WAVA_ITEM,
    DUTY_ITEM,
    MENU_ITEMS_COUNT
};

enum WaveForm
{
    SQUARE_WAVE,
    SINE_WAVE,
    TRIANGLE_WAVE,
    SAWTOOTH_WAVE,
    WAVES_COUNT
};

struct display_msg
{
  String text;
};

struct msg
{
  int p;
  display_msg &msg;
};

enum box_alignment
{
  Center,
  Left,
  Right
};
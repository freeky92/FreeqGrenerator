#include "tmr.h"
#include "config.h"
#include <Arduino.h>
#include <Data.h>
#include <EEManager.h>
#include <EncButton.h>
#include <GyverOLED.h>

Data data;
EEManager memory(data);
GyverOLED<SSD1306_128x64> oled;
EncButton encoder(ENC_S1, ENC_S2, ENC_BTN);

Tmr main_tmr;

// PWM параметры
const int pwmChannel = 0;
const int minimalFrqValue = 1;
const long maximumFrqValue = 313000;

MenuItem currentItem = STATUS_ITEM;
WaveForm currentWaveForm = SQUARE_WAVE;

bool editMode = false; // Режим редактирования (внутреннее значение)

bool pwmState = false; // Состояние генератора

void drawLogo()
{
  oled.line(10, 6, 86, 6);
  oled.line(10, 6, 10, 32);

  oled.line(37, 57, 117, 57);
  oled.line(117, 57, 117, 30);

  oled.setScale(2);
  oled.setCursorXY(17, 14);
  oled.print(F("Techno"));
  oled.setCursorXY(53, 35);
  oled.print(F("Touch"));
  oled.setScale(1);
  oled.update();
}

/**
 * Сущность области
 */
struct box
{
  int32_t x0, y0, x1, y1;
  /**
   * Метод очищает поле бокса
   */
  void clear()
  {
    oled.clear(x0, y0, x1, y1);
  }
  /**
   * Метод печатает текст маленькие
   */
  void printSmall(String str, box_alignment align, bool inverse = false)
  {
    int32_t left = x0;
    int32_t top = y0;
    int32_t width = x1 - x0;
    int32_t height = y1 - y0;

    int x = 0;
    if (align == Center)
    {
      x = left + 0.5 * (width - (str.length() * 6 - 1));
    }
    else if (align == Left)
    {
      x = left + 5;
    }
    else if (align == Right)
    {
      x = left + width - str.length() * 6 - 5;
    }
    oled.setCursorXY(x, top + 0.333 * height);
    oled.invertText(inverse);
    oled.print(str);
  }
  /**
   * Метод печатает текст Большые буквы
   */
  void printLarge(String str, bool inverse = false)
  {
    int32_t left = x0;
    int32_t top = y0;
    int32_t width = x1 - x0;
    int32_t height = y1 - y0;

    int x = left + 0.5 * (width - (str.length() * 12 - 1));

    oled.setScale(2);
    oled.setCursorXY(x, top + 0.333 * height);
    oled.invertText(inverse);
    oled.print(str);
    oled.setScale(1);
  }
  void drawBorder()
  {
    oled.roundRect(x0, y0, x1, y1, OLED_STROKE);
  }
};

/**
 * Боксы
 */
// Статус
box statusValue = {98, 1, 127, 15}; // Значение ON/OFF
// Частота
box freqTitle = {1, 1, 96, 15};    // Заголовок частоты
box freqValue = {1, 18, 82, 40};   // Значение частоты до 6 знаков
box freqUnits = {84, 18, 127, 40}; // Единицы измерения (Hz) до 3 знаков
// Скважность сигнала
box dutyTitle = {30, 42, 64, 63};   // Заголовок скважности
box dutyValue = {66, 42, 100, 63};  // Значение скважности
box dutyUnits = {102, 42, 127, 63}; // Единицы измерения (%)

void printFormatedFrequency(uint32_t fzh, bool inverse = false)
{
  if (fzh >= 1000000)
  {
    // Для значений от 1 МГц и выше
    if (fzh % 1000000 == 0)
    {
      freqValue.printLarge(String(fzh / 1000000), inverse);
      freqUnits.printLarge("MHz");
    }
    else
    {
      freqValue.printLarge(String(fzh / 1000000.0, 2), inverse);
      freqUnits.printLarge("MHz");
    }
  }
  else if (fzh >= 1000)
  {
    // Для значений от 1 кГц до 999.9 кГц
    if (fzh % 1000 == 0)
    {
      freqValue.printLarge(String(fzh / 1000), inverse);
      freqUnits.printLarge("kHz");
    }
    else
    {
      freqValue.printLarge(String(fzh / 1000.0, 2), inverse);
      freqUnits.printLarge("kHz");
    }
  }
  else
  {
    // Для значений ниже 1 кГц
    freqValue.printLarge(String(fzh), inverse);
    freqUnits.printLarge("Hz");
  }
}

void updScreen(bool state, int ghz, int d)
{
  oled.clear();

  statusValue.printSmall(pwmState ? "on" : "off", Center, pwmState);

  freqTitle.printSmall(F("Frequency"), Center);
  if (editMode && currentItem == FREQ_ITEM)
  {
    printFormatedFrequency(data.freq, true);
  }
  else
  {
    printFormatedFrequency(data.freq);
  }

  /*
  if (editMode && currentItem == HZ_ITEM)
  {
    printFormatedFrequency(freq, true);
  }
  else
  {
    printFormatedFrequency(freq);
  }
  */

  dutyTitle.printSmall(F("Duty:"), Center);
  if (editMode && currentItem == DUTY_ITEM)
  {
    dutyValue.printSmall(String(data.duty), Center, true);
  }
  else
  {
    dutyValue.printSmall(String(data.duty), Center);
  }
  dutyUnits.printSmall(F("%"), Center);

  switch (currentItem)
  {
  case STATUS_ITEM:
    statusValue.drawBorder();
    break;
  case FREQ_ITEM:
    freqValue.drawBorder();
    break;
  case HZ_ITEM:
    freqUnits.drawBorder();
    break;
  case DUTY_ITEM:
    dutyValue.drawBorder();
    break;
  }

  oled.update();
}

void updatePWMSignal()
{
  if (pwmState)
  {
    ledcChangeFrequency(pwmChannel, data.freq, 8);
    ledcWrite(pwmChannel, data.duty * 255 / 100);
  }
  else
  {
    ledcWrite(pwmChannel, 0);
  }
}

void updateState()
{
  updatePWMSignal();
  updScreen(pwmState, data.freq, data.duty);
}

void setEeprom()
{
  EEPROM.begin(memory.blockSize());
  memory.begin(0, 'b');
}

void setPWM()
{
  ledcSetup(pwmChannel, data.freq, 8);
  ledcAttachPin(PWM_PIN, pwmChannel);
}

void setDisplay()
{
  oled.init();
  oled.clear();
  oled.update();
}

void encoderEvents()
{
  // Прокручивание энкодера
  switch (encoder.action())
  {
  case EB_TURN:
    if (editMode)
    {
      int step = 1;
      switch (currentItem)
      {
      case FREQ_ITEM:
        if (data.freq >= 1000000)
        {
          step = encoder.encHolding() ? 100000 : 10000;
        }
        if (data.freq >= 1000 && data.freq < 1000000)
        {
          step = encoder.encHolding() ? 1000 : 100;
        }
        if (data.freq < 1000)
        {
          step = encoder.encHolding() ? 100 : 1;
        }

        data.freq += encoder.dir() * step;
        Serial.println(data.freq);

        break;
      case HZ_ITEM:
        if (encoder.dir() == 1)
        {
          data.freq *= 1000;
        }
        else if (encoder.dir() == -1)
        {
          data.freq /= 1000;
        }

        break;
      case DUTY_ITEM:
        data.duty += encoder.dir() * 5;
        data.duty = constrain(data.duty, 5, 95);
        data.duty = data.duty - (data.duty % 5); // Фиксация шага 5%

        break;
      }

      data.freq = constrain(data.freq, minimalFrqValue, maximumFrqValue);
      Serial.println(data.freq);

      updateState();
    }
    else
    {
      if (encoder.dir() == 1)
      {
        Serial.println("1");
        currentItem = static_cast<MenuItem>((currentItem + 1) % MENU_ITEMS_COUNT);
      }
      else if (encoder.dir() == -1)
      {
        Serial.println("-1");
        currentItem = static_cast<MenuItem>((currentItem - 1 + MENU_ITEMS_COUNT) % MENU_ITEMS_COUNT);
      }
      updScreen(pwmState, data.freq, data.duty);
    }
    break;

  // нажание энкодера
  case EB_CLICK:
    if (currentItem == STATUS_ITEM)
    {
      pwmState = !pwmState;
      updScreen(pwmState, data.freq, data.duty);
    }
    else
    {
      editMode = !editMode;
      if (!editMode)
      {
        memory.updateNow();
      }
      Serial.println("editMode: " + String(editMode));
    }
    updateState();

    break;

  default:
    break;
  }
}

// ========================= SETUP =========================
void setup()
{
  Serial.begin(115200);
  Serial.println(F("Start"));

  setEeprom();
  setDisplay();
  setPWM();

  // ========================= LOGO =========================
  drawLogo();
  delay(500);

  encoder.attach(encoderEvents);
  updateState();
}

// ========================= LOOP =========================
void loop()
{
  encoder.tick();
  main_tmr.tick();
}
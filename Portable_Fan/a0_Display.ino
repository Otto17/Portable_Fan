// Copyright (c) 2025 Otto
// Лицензия: MIT (см. LICENSE)


// --- Макросы работы с портом 'PC' ---
#define SET_INPUT(ddr, bit) ((ddr) &= ~(1 << (bit)))
#define SET_OUTPUT(ddr, bit) ((ddr) |= (1 << (bit)))
#define SET_HIGH(port, bit) ((port) |= (1 << (bit)))
#define SET_LOW(port, bit) ((port) &= ~(1 << (bit)))

// --- Порты PC0–PC4' ---
#define SEG_A PC0
#define SEG_B PC1
#define SEG_C PC2
#define SEG_D PC3
#define SEG_E PC4

const uint8_t segmentPins[] = { SEG_A, SEG_B, SEG_C, SEG_D, SEG_E };

// Сегментная карта цифр 0–9
const uint8_t digitSegments[10][7] = {
  { 1, 1, 1, 0, 1, 1, 1 },  // 0
  { 0, 0, 1, 0, 0, 1, 0 },  // 1
  { 1, 0, 1, 1, 1, 0, 1 },  // 2
  { 1, 0, 1, 1, 0, 1, 1 },  // 3
  { 0, 1, 1, 1, 0, 1, 0 },  // 4
  { 1, 1, 0, 1, 0, 1, 1 },  // 5
  { 1, 1, 0, 1, 1, 1, 1 },  // 6
  { 1, 0, 1, 0, 0, 1, 0 },  // 7
  { 1, 1, 1, 1, 1, 1, 1 },  // 8
  { 1, 1, 1, 1, 0, 1, 1 }   // 9
};

// Пары GND/VCC для сегментов
struct SegmentPair {
  uint8_t gnd, vcc;
};

// Пары GND/VCC для 1-й цифры
const SegmentPair segmentMap1[7] = {
  { PC0, PC1 },  // Сегмент a
  { PC0, PC2 },  // Сегмент b
  { PC1, PC0 },  // Сегмент c
  { PC2, PC0 },  // Сегмент d
  { PC1, PC2 },  // Сегмент e
  { PC0, PC3 },  // Сегмент f
  { PC1, PC3 }   // Сегмент g
};

// Пары GND/VCC для 2-й цифры
const SegmentPair segmentMap2[7] = {
  { PC2, PC1 }, { PC2, PC4 }, { PC1, PC2 }, { PC3, PC4 }, { PC1, PC4 }, { PC2, PC3 }, { PC1, PC3 }
};

// Пары GND/VCC для 3-й цифры
const SegmentPair segmentMap3[7] = {
  { PC1, PC0 }, { PC0, PC3 }, { PC0, PC1 }, { PC0, PC4 }, { PC3, PC0 }, { PC2, PC0 }, { PC0, PC2 }
};

// Структура для хранения сегментов каждой цифры
struct DigitDisplay {
  uint8_t numSegments;
  SegmentPair segments[7];  // Макс. 7 сегментов
};

DigitDisplay displayDigits[4];  // 3 цифры + 1 сегмент температуры

// Состояние отрисовки
struct StateDraw {
  uint8_t digit;         // 1..4
  uint8_t segment;       // Индекс в displayDigits[digit-1].segments
  uint16_t value;        // 0..199
  bool showTempSegment;  // Флаг для сегмента температуры
} draw;

// Очистка всех сегментов (PC0..PC4)
void clearAllSegments() {
  DDRC &= ~0x1F;   // Установить PC0-PC4 как входы
  PORTC &= ~0x1F;  // Отключить подтягивающие резисторы
}

// Зажечь один сегмент
void lightSeg(uint8_t gnd, uint8_t vcc) {
  // Линии уже очищены
  SET_OUTPUT(DDRC, gnd);
  SET_LOW(PORTC, gnd);
  SET_OUTPUT(DDRC, vcc);
  SET_HIGH(PORTC, vcc);
}


// Устанавливает число на дисплее
void setDisplayNumber(uint16_t n, bool showTemp = false) {
  if (n > 199) return;  // Максимальное число 199, которое возможно вывести
  draw.value = n;
  draw.showTempSegment = showTemp;

  // Мультиплексинг цифр
  uint8_t d1 = n / 100;
  uint8_t d2 = (n / 10) % 10;
  uint8_t d3 = n % 10;

  // Цифра 1
  if (d1 == 1) {
    displayDigits[0].numSegments = 2;
    displayDigits[0].segments[0] = segmentMap1[2];  // Сегмент c
    displayDigits[0].segments[1] = segmentMap1[5];  // Сегмент f
  } else if (d1 > 0) {
    displayDigits[0].numSegments = 0;
    for (uint8_t s = 0; s < 7; s++) {
      if (digitSegments[d1][s]) {
        displayDigits[0].segments[displayDigits[0].numSegments++] = segmentMap1[s];
      }
    }
  } else {
    displayDigits[0].numSegments = 0;
  }

  // Цифра 2
  if (n >= 10) {
    displayDigits[1].numSegments = 0;
    for (uint8_t s = 0; s < 7; s++) {
      if (digitSegments[d2][s]) {
        displayDigits[1].segments[displayDigits[1].numSegments++] = segmentMap2[s];
      }
    }
  } else {
    displayDigits[1].numSegments = 0;
  }

  // Цифра 3
  displayDigits[2].numSegments = 0;
  for (uint8_t s = 0; s < 7; s++) {
    if (digitSegments[d3][s]) {
      displayDigits[2].segments[displayDigits[2].numSegments++] = segmentMap3[s];
    }
  }

  // Сегмент температуры
  if (showTemp) {
    displayDigits[3].numSegments = 1;
    displayDigits[3].segments[0] = { PC3, PC2 };  // Сегмент температуры
  } else {
    displayDigits[3].numSegments = 0;
  }

  draw.digit = 1;
  draw.segment = 0;
}

// Очищает дисплей (все пины INPUT)
void clearDisplay() {
  for (uint8_t i = 0; i < 5; i++) {
    SET_INPUT(DDRC, segmentPins[i]);  // Отключает как выход
    SET_LOW(PORTC, segmentPins[i]);   // Снимает pull-up
  }
}

// Настройка мультидисплея с Timer2
void setup_MultiDisplay() {
  clearDisplay();
  draw = { 1, 0, 0, false };

  // Настройка Timer2 для ATmega8L-8AU
  TCCR2 = (1 << WGM21) | (1 << CS22);  // Режим CTC, предделитель 64
  OCR2 = 74;                           // 600 мкс при 8 МГц
  TIMSK |= (1 << OCIE2);               // Включить прерывание по совпадению
}

// Обработчик прерывания Timer2
ISR(TIMER2_COMP_vect) {
  clearAllSegments();

  if (draw.digit <= 4 && draw.segment < displayDigits[draw.digit - 1].numSegments) {
    SegmentPair pair = displayDigits[draw.digit - 1].segments[draw.segment];
    lightSeg(pair.gnd, pair.vcc);
  }

  draw.segment++;
  if (draw.segment >= displayDigits[draw.digit - 1].numSegments) {
    draw.segment = 0;
    do {
      draw.digit = (draw.digit % 4) + 1;
    } while (displayDigits[draw.digit - 1].numSegments == 0 && draw.digit != 1);
  }
}
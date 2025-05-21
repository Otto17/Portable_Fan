// Copyright (c) 2025 Otto
// Лицензия: MIT (см. LICENSE)


// Измерения питания "ATmega8L-8AU" и расчёт уровня заряда Li-ion аккумулятора в процентах
uint8_t Battery() {
  static uint32_t lastUpdateMicros = 0;  // Таймер для обновления измерений (что бы не тратить много процессорного времени на АЦП)
  static uint8_t percent = 0;            // Возвращает % заряда аккумулятора

  // Обновление "percent" раз в 3 секунды, остальное время метод "Battery()" просто возвращает предыдущий сохранённый результат
  if ((uint32_t)(micros() - lastUpdateMicros) >= 3000000) {
    lastUpdateMicros = micros();

    // Настраиваем АЦП: измеряем внутреннее опорное напряжение (~1.1V – 1.3V) относительно Vcc
    ADMUX = (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);

    long buffersamp = 0;
    for (int n = 0x0; n <= 0xff; n++) {
      ADCSRA |= (1 << ADSC) | (1 << ADEN);  // Начинаем новую конверсию

      // Ждём завершения
      while (bit_is_set(ADCSRA, ADSC))
        ;
      buffersamp += ADC;
    }
    buffersamp >>= 8;        // Усредняем 256 выборок (делим на 256), максимальное значение АЦП — 1023 (10 бит)
    ADCSRA &= ~(1 << ADEN);  // Отключаем АЦП

    // Вычисляем Vcc по формуле: Vcc = Vref * 1023 / ADC_среднее
    float Vin = (float)(1.305 * 1023) / buffersamp;  // Vref подбирается вручную (например, 1.305 В) под конкретный МК

    // Переводим напряжение Vin в процент заряда (2.9 В → 0 %, 4.2 В → 100 %)
    if (Vin <= 2.9) percent = 0;
    else if (Vin >= 4.2) percent = 100;
    else percent = (uint8_t)(((Vin - 2.9) / (4.2 - 2.9)) * 100.0 + 0.5);  // Округление
  }

  return percent;
}
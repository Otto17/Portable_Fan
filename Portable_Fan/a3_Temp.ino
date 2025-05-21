// Copyright (c) 2025 Otto
// Лицензия: MIT (см. LICENSE)


// Функция измерения температуры с блокировкой отрицательных значений
uint8_t Temp() {
  static uint32_t lastUpdateMicros = 0;  // Время последнего измерения
  static uint8_t celsius = 0;            // Последнее вычисленное значение температуры
  static uint32_t powerOnMicros = 0;     // Время подачи питания на делитель
  static bool waiting = false;           // Флаг ожидания стабилизации

  uint32_t now = micros();

  // Если ожидание после включения делителя — ждём стабилизации
  if (waiting) {
    if ((uint32_t)(now - powerOnMicros) < 2000) return celsius;  // Ждём 2 мс без delay
    waiting = false;                                             // Стабилизация завершена

    // Запускаем ADC на аналоговом канале 6 (A6):
    ADMUX = (1 << REFS0)                                    // Опорное напряжение AVcc
            | (1 << MUX2) | (1 << MUX1);                    // Выбираем ADC6 (MUX=0110)
    ADCSRA = (1 << ADEN)                                    // Включаем АЦП
             | (1 << ADSC)                                  // Начинаем преобразование
             | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Делитель частоты 128
    while (ADCSRA & (1 << ADSC))
      ;                  // Ждём окончания преобразования
    uint16_t adc = ADC;  // Считываем результат

    // Выключаем питание делителя напряжения
    PORTB &= ~(1 << PB0);  // PB0 = LOW (убираем питание с делителя)
    DDRB &= ~(1 << PB0);   // PB0 = INPUT (ВХОД)

    // Формулы расчёта температуры:
    float voltage = adc * 1.305f / 1023.0f;                                     // Перевод АЦП в напряжение (U = ADC * Vref / 1023)
    float r_ntc = 10000.0f * voltage / (1.305f - voltage);                      // Закон делителя напряжения (Rntc = R * U / (Vref - U))
    float tK = 1.0f / ((1 / 298.15f) + (1 / 3950.0f) * log(r_ntc / 10000.0f));  // Формула Стейнхарта–Харта
    float tC = tK - 273.15f;                                                    // Перевод Кельвинов °C

    int tInt = (int)(tC + 0.5f);  // Округление
    if (tInt < 0) tInt = 0;       // Блокировка отрицательных температур
    celsius = (uint8_t)tInt;
  }

  // Обновление раз в 3 секунды — остальное время возвращаем старое значение
  if ((uint32_t)(now - lastUpdateMicros) >= 3000000 && !waiting) {
    lastUpdateMicros = now;

    // Включаем питание делителя напряжения
    DDRB |= (1 << PB0);   // PB0 = OUTPUT (ВЫХОД)
    PORTB |= (1 << PB0);  // PB0 = HIGH (подаём питание на делитель)

    powerOnMicros = now;
    waiting = true;  // Устанавливаем флаг ожидания
  }

  return celsius;
}


// Сбрасывает порты измерения температуры на ВХОД
void resetPortTemp() {
  PORTB &= ~(1 << PB0);  // PB0 = LOW (убираем питание с делителя)
  DDRB &= ~(1 << PB0);   // PB0 = INPUT (ВХОД)
}
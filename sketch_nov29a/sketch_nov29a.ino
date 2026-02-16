#include <Arduino.h>

// ПИНЫ ДЛЯ СДВИГОВЫХ РЕГИСТРОВ

// Первый сдвиговый регистр управляет светофорами и пищалками
const int dataPin1 = 11;
const int latchPin1 = 10;
const int clockPin1 = 12;

// Второй сдвиговый регистр управляет таймером A
const int dataPin2 = 9;
const int latchPin2 = 7;
const int clockPin2 = 8;

// Третий сдвиговый регистр управляет таймером B
const int dataPin3 = 6;
const int latchPin3 = 4;
const int clockPin3 = 5;

// ПИН КНОПКИ
const int buttonPin = 3;   // Кнопка для переключения режимов

// ПЕРЕЧИСЛЕНИЕ РЕЖИМОВ РАБОТЫ
enum Mode {
  MODE_STANDARD,    // Нормальная работа светофора
  MODE_BLINK_YELLOW // Режим мигания желтым светом на обоих светофорах
};

Mode currentMode = MODE_STANDARD;
bool buttonPressed = false;        // Флаг нажатия кнопки
bool lastButtonState = HIGH;       // Предыдущее состояние кнопки

// ТАЙМИНГИ В СЕКУНДАХ
const int GREEN_TIME = 6;    // Длительность зеленого
const int YELLOW_TIME = 2;   // Длительность желтого
const int RED_TIME = 9;      // Длительность красного

// ПЕРЕМЕННЫЕ СОСТОЯНИЯ
int timerA = GREEN_TIME;     // Таймер для светофора A (начинает с зеленого)
int timerB = RED_TIME;       // Таймер для светофора B (начинает с красного)
unsigned long lastSecondTime = 0;  // Время последнего обновления секунды
unsigned long lastBlinkTime = 0;   // Время последнего мигания
bool greenBlink = false;     // Флаг для мигания зеленого
bool yellowBlink = false;    // Флаг для мигания желтого

// СОСТОЯНИЯ СВЕТОФОРА A
bool a_green = true;         // Зеленый свет включен
bool a_yellow = false;       // Желтый свет выключен
bool a_red = false;          // Красный свет выключен
bool a_red_yellow = false;   // Красно-желтый режим выключен

// СОСТОЯНИЯ СВЕТОФОРА B
bool b_green = false;        // Зеленый свет выключен
bool b_yellow = false;       // Желтый свет выключен
bool b_red = true;           // Красный свет включен
bool b_red_yellow = false;   // Красно-желтый режим выключен

// КАРТА ДЛЯ 7-СЕГМЕНТНОГО ИНДИКАТОРА
const byte digitMap[10] = {
  0b00111111, // Цифра 0
  0b00000110, // Цифра 1
  0b01011011, // Цифра 2
  0b01001111, // Цифра 3
  0b01100110, // Цифра 4
  0b01101101, // Цифра 5
  0b01111101, // Цифра 6
  0b00000111, // Цифра 7
  0b01111111, // Цифра 8
  0b01101111  // Цифра 9
};

void setup() {
  // Настройка пинов для управления сдвиговыми регистрами
  pinMode(latchPin1, OUTPUT);
  pinMode(clockPin1, OUTPUT);
  pinMode(dataPin1, OUTPUT);
  
  pinMode(latchPin2, OUTPUT);
  pinMode(clockPin2, OUTPUT);
  pinMode(dataPin2, OUTPUT);
  
  pinMode(latchPin3, OUTPUT);
  pinMode(clockPin3, OUTPUT);
  pinMode(dataPin3, OUTPUT);
  
  // Настройка пинов кнопки
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Инициализация временных меток
  lastSecondTime = millis();
  lastBlinkTime = millis();
  
  // Первоначальное обновление всех индикаторов
  updateAllDisplays();
}

// ОСНОВНОЙ ЦИКЛ
void loop() {
  unsigned long currentTime = millis(); // Текущее время в миллисекундах
  
  // Обработка нажатия кнопки
  handleButton();
  
  // Выбор режима работы в зависимости от currentMode
  if (currentMode == MODE_STANDARD) {
    standardMode(currentTime); // Запуск стандартного режима
  } else {
    blinkYellowMode(currentTime); // Запуск режима мигания желтым
  }
}

// ОБРАБОТКА КНОПКИ
void handleButton() {
  bool currentButtonState = digitalRead(buttonPin); // Чтение состояния кнопки
  
  // Обнаружение нажатия
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Задержка для устранения дребезга контактов
    // Повторная проверка после задержки
    if (digitalRead(buttonPin) == LOW) {
      // Переключение между двумя режимами
      currentMode = (currentMode == MODE_STANDARD) ? MODE_BLINK_YELLOW : MODE_STANDARD;
      
      // Сброс состояний при переключении режима
      if (currentMode == MODE_STANDARD) {
        resetToStandardMode(); // Вернуться в начальное состояние стандартного режима
      } else {
        // При переходе в режим мигания выключаем все светофоры
        digitalWrite(latchPin1, LOW);
        shiftOut(dataPin1, clockPin1, MSBFIRST, 0); // Передаем 0 (все выключено)
        digitalWrite(latchPin1, HIGH);
        updateTimerDisplays(0, 0); // Сбрасываем таймеры на 0
      }
    }
  }
  
  lastButtonState = currentButtonState; // Сохраняем состояние для следующего цикла
}

// СТАНДАРТНЫЙ РЕЖИМ РАБОТЫ СВЕТОФОРА
void standardMode(unsigned long currentTime) {
  // Проверяем, прошла ли 1 секунда для обновления таймеров
  if (currentTime - lastSecondTime >= 1000) {
    lastSecondTime = currentTime; // Обновляем метку времени
    
    // Уменьшаем таймеры каждую секунду
    timerA--;
    timerB--;
    
    // Проверяем нужно ли сменить состояние светофоров
    checkStates();
    
    // Обновляем светофоры и таймеры
    updateAllDisplays();
  }
  
  // Мигание зеленого каждые 500 мс
  if (currentTime - lastBlinkTime >= 500) {
    lastBlinkTime = currentTime;
    greenBlink = !greenBlink; // Инвертируем флаг мигания
    updateAllDisplays(); // Обновляем отображение
  }
}

// РЕЖИМ МИГАНИЯ ЖЕЛТЫМ
void blinkYellowMode(unsigned long currentTime) {
  // Мигание желтым сигналом каждые 500 мс
  if (currentTime - lastBlinkTime >= 500) {
    lastBlinkTime = currentTime;
    yellowBlink = !yellowBlink; // Инвертируем флаг мигания
    
    byte data = 0; // Переменная для данных отправляемых в регистр
    if (yellowBlink) {
      data |= (1 << 1); // Включаем желтый, светофор A (бит 1)
      data |= (1 << 5); // Включаем желтый, светофор B (бит 5)
    }
    
    // Отправляем данные в сдвиговый регистр светофоров
    digitalWrite(latchPin1, LOW);
    shiftOut(dataPin1, clockPin1, MSBFIRST, data);
    digitalWrite(latchPin1, HIGH);
    
    // В режиме мигания показываем 0 на таймерах
    updateTimerDisplays(0, 0);
  }
}

// СБРОС В СТАНДАРТНЫЙ РЕЖИМ
void resetToStandardMode() {
  // Восстанавливаем начальные значения таймеров
  timerA = GREEN_TIME;
  timerB = RED_TIME;
  
  // Сбрасываем состояния светофора A в начальные состояния
  a_green = true;
  a_yellow = false;
  a_red = false;
  a_red_yellow = false;
  
  // Сбрасываем состояния светофора B в начальные состояния
  b_green = false;
  b_yellow = false;
  b_red = true;
  b_red_yellow = false;
  
  // Обновляем временные метки
  lastSecondTime = millis();
  
  // Обновляем все дисплеи
  updateAllDisplays();
}

// ПРОВЕРКА И СМЕНА СОСТОЯНИЙ СВЕТОФОРОВ
void checkStates() {
  // ЛОГИКА ДЛЯ СВЕТОФОРА A
  if (a_green) { // Если горит зеленый
    if (timerA <= 0) { // Таймер истек
      a_green = false;   // Выключаем зеленый
      a_yellow = true;   // Включаем желтый
      timerA = YELLOW_TIME; // Устанавливаем таймер для желтого
    }
  } else if (a_yellow) { // Если горит желтый
    if (timerA <= 0) { // Таймер истек
      a_yellow = false;  // Выключаем желтый
      a_red = true;      // Включаем красный
      timerA = RED_TIME; // Устанавливаем таймер для красного
    }
  } else if (a_red) { // Если горит красный
    if (timerA <= 2) { // Осталось 2 секунды или меньше
      a_red = false;        // Выключаем красный
      a_red_yellow = true;  // Включаем красно-желтый
    }
  } else if (a_red_yellow) { // Если горит красно-желтый
    if (timerA <= 0) { // Таймер истек
      a_red_yellow = false; // Выключаем красно-желтый
      a_green = true;       // Включаем зеленый
      timerA = GREEN_TIME;  // Устанавливаем таймер для зеленого
    }
  }
  
  // ЛОГИКА ДЛЯ СВЕТОФОРА B (аналогична светофору A)
  if (b_green) {
    if (timerB <= 0) {
      b_green = false;
      b_yellow = true;
      timerB = YELLOW_TIME;
    }
  } else if (b_yellow) {
    if (timerB <= 0) {
      b_yellow = false;
      b_red = true;
      timerB = RED_TIME;
    }
  } else if (b_red) {
    if (timerB <= 2) {
      b_red = false;
      b_red_yellow = true;
    }
  } else if (b_red_yellow) {
    if (timerB <= 0) {
      b_red_yellow = false;
      b_green = true;
      timerB = GREEN_TIME;
    }
  }
}

// ОБНОВЛЕНИЕ ВСЕХ ИНДИКАТОРОВ
void updateAllDisplays() {
  updateTrafficLights(); // Обновляем светофоры и пищалки
  updateTimerDisplays(timerA, timerB); // Обновляем таймеры
}

// ОБНОВЛЕНИЕ СВЕТОФОРОВ И ПИЩАЛОК
void updateTrafficLights() {
  byte data = 0; // Переменная для формирования данных
  
  // УПРАВЛЕНИЕ СВЕТОФОРОМ A
  if (a_green) { // Зеленый режим
    if (timerA > 2) { // Если больше 2 секунд - горит постоянно
      data |= (1 << 2); // Включаем зеленый свет (бит 2)
    } else { // Последние 2 секунды - мигает
      if (greenBlink) { // Мигание по флагу
        data |= (1 << 2); // Включаем зеленый на полсекунды
      }
    }
    data |= (1 << 3); // Включаем пищалку A (бит 3)
  } else if (a_yellow) { // Желтый режим
    data |= (1 << 1); // Включаем желтый свет (бит 1)
  } else if (a_red) { // Красный режим
    data |= (1 << 0); // Включаем красный свет (бит 0)
  } else if (a_red_yellow) { // Красно-желтый режим
    data |= (1 << 0); // Красный
    data |= (1 << 1); // Желтый
  }
  
  // УПРАВЛЕНИЕ СВЕТОФОРОМ B
  if (b_green) {
    if (timerB > 2) {
      data |= (1 << 6); // Зеленый B (бит 6)
    } else {
      if (greenBlink) {
        data |= (1 << 6);
      }
    }
    data |= (1 << 7); // Пищалка B (бит 7)
  } else if (b_yellow) {
    data |= (1 << 5); // Желтый B (бит 5)
  } else if (b_red) {
    data |= (1 << 4); // Красный B (бит 4)
  } else if (b_red_yellow) {
    data |= (1 << 4); // Красный
    data |= (1 << 5); // Желтый
  }
  
  // ОТПРАВКА ДАННЫХ В СДВИГОВЫЙ РЕГИСТР
  digitalWrite(latchPin1, LOW); // Начинаем передачу
  shiftOut(dataPin1, clockPin1, MSBFIRST, data); // Передаем байт данных
  digitalWrite(latchPin1, HIGH); // Завершаем передачу, выводим данные на выходы
}

// ОБНОВЛЕНИЕ ЦИФРОВЫХ ТАЙМЕРОВ
void updateTimerDisplays(int valueA, int valueB) {
  // Обновление таймера A (если значение от 0 до 9)
  if (valueA >= 0 && valueA <= 9) {
    digitalWrite(latchPin2, LOW);
    shiftOut(dataPin2, clockPin2, MSBFIRST, digitMap[valueA]); // Передаем код цифры
    digitalWrite(latchPin2, HIGH);
  }
  
  // Обновление таймера B (если значение от 0 до 9)
  if (valueB >= 0 && valueB <= 9) {
    digitalWrite(latchPin3, LOW);
    shiftOut(dataPin3, clockPin3, MSBFIRST, digitMap[valueB]);
    digitalWrite(latchPin3, HIGH);
  }
}
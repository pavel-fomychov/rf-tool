#include <EEPROM.h>
#include "OneButton.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
SSD1306AsciiWire oled;

#define rxPin 2                         // Приемник
#define rxOn 3                          // Включение приёмника
#define txPin 4                         // Передатчик
#define ledCach1 8                      // Индикатор кеша 1
#define ledCach2 6                      // Индикатор кеша 2
#define ledJammer 7                     // Индикатор глушилки
#define btsendPin1 A1                   // кнопка 1
#define btsendPin2 A2                   // кнопка 2
#define bip A0                          // Вибро
#define pulseAN 412                     // длительность импульса AN-Motors
#define maxDelta 200                    // максимальное отклонение от длительности при приеме

OneButton button1(btsendPin1, false);   // вызов функции отслеживания кнопка 1
OneButton button2(btsendPin2, false);   // вызов функции отслеживания кнопка 2

volatile unsigned long prevtime;
volatile unsigned int lolen, hilen, state;
int clickCash;

//AN MOTORS
volatile static byte bcounter = 0;      // количество принятых битов
volatile static long code1 = 0;         // зашифрованная часть
volatile static long code2 = 0;         // фиксированная часть
volatile long c1 = 0;                   // переменная для отправки
volatile long c2 = 0;                   // переменная для отправки
volatile long Cash1 = 0;                // кеш переменная для отправки
volatile long Cash2 = 0;                // кеш переменная для отправки
int CashTrigger = 1;                    // переключение между перемеными кеша

//CAME
volatile static byte cameCounter = 0;   // сохраненое количество бит
volatile static long cameCode = 0;      // код Came
volatile long cashCame1 = 0;            // кеш переменная для отправки
volatile long cashCame2 = 0;            // кеш переменная для отправки
int cashCameTrigger = 1;                // переключение между перемеными кеша

//NICE
volatile static byte niceCounter = 0;   // сохраненое количество бит
volatile static long niceCode = 0;      // код Nice
volatile long cashNice1 = 0;            // кеш переменная для отправки
volatile long cashNice2 = 0;            // кеш переменная для отправки
int cashNiceTrigger = 1;                // переключение между перемеными кеша

//DISPLAY
String displayTx = "";                  // кеш дисплея передача
String displayRx = "";                  // кеш дисплея прием
boolean displayClear = true;            // первичная очистка дисплея

unsigned long voltage = 0;              // процент заряда АКБ

void setup() {
  Serial.begin(9600);

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(rxOn, OUTPUT);
  digitalWrite(rxOn, HIGH);
  pinMode(bip, OUTPUT);
  pinMode(ledCach1, OUTPUT);
  pinMode(ledCach2, OUTPUT);
  pinMode(ledJammer, OUTPUT);
  pinMode(btsendPin1, INPUT);
  pinMode(btsendPin2, INPUT);

  cachView();                             // Индикатор кеша, запрос кеша из EEPROM
  attachInterrupt(0, grab, CHANGE);       // Перехват пакетов ( 1 для Pro Micro | 0 для Uno, Nano, Pro Mini )
  randomSeed(analogRead(0));              // Генерация случайного числа для AN-Motors

  button1.attachClick(click1);
  button1.attachDoubleClick(doubleclick1);
  button1.attachLongPressStart(longPressStart1);
  button2.attachClick(click2);
  button2.attachLongPressStart(longPressStart2);

  oled.begin(&Adafruit128x64, 0x3C);
  oled.clear();
  oled.setFont(Arial14);

  unsigned long lasttime = 0;
  while (true) {
    if (lasttime < millis()) {
      voltage = readVcc();  // Статус батареи
      oled.setCursor(29, 1);
      oled.println("RF - TOOLS");
      oled.setCursor(17, 10);
      oled.println("READY TO USE");
      oled.setCursor(28, 20);
      oled.print("battery: ");
      oled.print(voltage);
      oled.println("%  ");
      lasttime = millis() + 60000;
    }
    if (analogRead(btsendPin1) > 550 || analogRead(btsendPin2) > 550 || displayRx != "") {
      oled.setFont(font5x7);
      oled.setCursor(0, 0);
      oled.setScrollMode(SCROLL_MODE_AUTO);
      break;
    }
  }

}

void loop() {
  //Отслеживание нажатия кнопок
  button1.tick();
  button2.tick();

  //Вывод на дисплей принятых данных
  RxDisplay();
}

//Отправка кода из кеша 1
void click1() {
  clickCash = 1;
  cachView();
  if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, HIGH);
  }
  digitalWrite(rxOn, LOW);  // Выкл перехват
  if (Cash1 != 0) {
    c1 = 0x25250000 + random(0xffff);
    c2 = Cash1;
    SendANMotors(c1, c2);
  }
  if (cashCame1 != 0) {
    SendCame(cashCame1);
  }
  if (cashNice1 != 0) {
    SendNice(cashNice1);
  }
  TxDisplay();
  digitalWrite(rxOn, HIGH);  // Вкл перехват
  if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
    bipOne();
  }
  cachView();
}

//Отправка кода из кеша 2
void doubleclick1() {
  clickCash = 2;
  cachView();
  if (Cash2 != 0 || cashCame2 != 0 || cashNice2 != 0) {
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, HIGH);
  }
  digitalWrite(rxOn, LOW);  // Выкл перехват
  if (Cash2 != 0) {
    c1 = 0x25250000 + random(0xffff);
    c2 = Cash2;
    SendANMotors(c1, c2);
  }
  if (cashCame2 != 0) {
    SendCame(cashCame2);
  }
  if (cashNice2 != 0) {
    SendNice(cashNice2);
  }
  TxDisplay();
  digitalWrite(rxOn, HIGH);  // Вкл перехват
  if (Cash2 != 0 || cashCame2 != 0 || cashNice2 != 0) {
    bipTwo();
  }
  cachView();
}

//Сохранение в энергонезависимую память EEPROM
void longPressStart1() {
  digitalWrite(ledJammer, HIGH);
  digitalWrite(ledCach1, HIGH);
  digitalWrite(ledCach2, HIGH);
  int checkCash = 0;

  if (Cash1 != 0) {
    EEPROM.put(0, Cash1);
    checkCash = 1;
  }
  if (Cash2 != 0) {
    EEPROM.put(10, Cash2);
    checkCash = 1;
  }
  if (cashCame1 != 0) {
    EEPROM.put(20, cashCame1);
    checkCash = 1;
  }
  if (cashCame2 != 0) {
    EEPROM.put(30, cashCame2);
    checkCash = 1;
  }
  if (cashNice1 != 0) {
    EEPROM.put(40, cashNice1);
    checkCash = 1;
  }
  if (cashNice2 != 0) {
    EEPROM.put(50, cashNice2);
    checkCash = 1;
  }

  if (checkCash == 1) {
    clearDisplay();
    oled.println("Ms: Save to EEPROM");
  } else {
    digitalWrite(ledCach1, LOW);
    clearDisplay();
    oled.println("Ms: No data to save");
  }
  bipLong(false);
  digitalWrite(ledJammer, LOW);
  cachView();
}

//Вкл/откл глушилки
void click2() {
  bipOne();
  digitalWrite(ledJammer, HIGH);
  digitalWrite(ledCach1, LOW);
  digitalWrite(ledCach2, LOW);
  digitalWrite(rxOn, LOW);                // Выкл перехват
  clearDisplay();
  oled.println("Ms: Jammer active");
  while (true) {
    digitalWrite(txPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(txPin, LOW);
    delayMicroseconds(500);
    if (analogRead(btsendPin2) > 550) {
      clearDisplay();
      oled.println("Ms: Jammer stop");
      digitalWrite(ledJammer, LOW);
      bipOne();
      cachView();
      delay(300);
      break;
    }
  }
  digitalWrite(rxOn, HIGH);               // Вкл перехват

}

//Очистка кеша
void longPressStart2() {
  Cash1 = 0;
  Cash2 = 0;
  cashCame1 = 0;
  cashCame2 = 0;
  cashNice1 = 0;
  cashNice2 = 0;
  for (int i = 0; i <= 70; i++) {
    EEPROM.put(i, 0);
  }
  clearDisplay();
  oled.println("Ms: Cache is cleared");
  digitalWrite(ledCach1, LOW);
  digitalWrite(ledCach2, LOW);
  bipLong(false);
}


//-----------------------------------------------------------------------------------------------------------------------------

//Передача

//AN-MOTORS
void SendANMotors(long c1, long c2) {
  for (int j = 0; j < 4; j++)  {
    //отправка 12 начальных импульсов 0-1
    for (int i = 0; i < 12; i++) {
      delayMicroseconds(pulseAN);
      digitalWrite(txPin, HIGH);
      delayMicroseconds(pulseAN);
      digitalWrite(txPin, LOW);
    }
    delayMicroseconds(pulseAN * 10);
    //отправка первой части кода
    for (int i = 32; i > 0; i--) {
      SendBit(bitRead(c1, i - 1), pulseAN);
    }
    //отправка второй части кода
    for (int i = 32; i > 0; i--) {
      SendBit(bitRead(c2, i - 1), pulseAN);
    }
    //отправка бит, которые означают батарею и флаг повтора
    SendBit(1, pulseAN);
    SendBit(1, pulseAN);
    delayMicroseconds(pulseAN * 39);
  }
  displayTx = String(c1, HEX) + " " + String(c2, HEX);
  displayTx.toUpperCase();
  TxDisplay();
}


void SendBit(byte b, int pulse) {
  if (b == 0) {
    digitalWrite(txPin, HIGH);        // 0
    delayMicroseconds(pulse * 2);
    digitalWrite(txPin, LOW);
    delayMicroseconds(pulse);
  }
  else {
    digitalWrite(txPin, HIGH);        // 1
    delayMicroseconds(pulse);
    digitalWrite(txPin, LOW);
    delayMicroseconds(pulse * 2);
  }
}

//CAME
void SendCame(long Code) {
  for (int j = 0; j < 4; j++) {
    digitalWrite(txPin, HIGH);
    delayMicroseconds(320);
    digitalWrite(txPin, LOW);
    for (int i = 12; i > 0; i--) {
      byte b = bitRead(Code, i - 1);
      if (b) {
        digitalWrite(txPin, LOW);     // 1
        delayMicroseconds(640);
        digitalWrite(txPin, HIGH);
        delayMicroseconds(320);
      } else {
        digitalWrite(txPin, LOW);     // 0
        delayMicroseconds(320);
        digitalWrite(txPin, HIGH);
        delayMicroseconds(640);
      }
    }
    digitalWrite(txPin, LOW);
    delayMicroseconds(11520);
  }
  displayTx = "Came " + String(Code & 0xfff);
  TxDisplay();
}

//NICE
void SendNice(long Code) {
  for (int j = 0; j < 4; j++) {
    digitalWrite(txPin, HIGH);
    delayMicroseconds(700);
    digitalWrite(txPin, LOW);
    for (int i = 12; i > 0; i--) {
      byte b = bitRead(Code, i - 1);
      if (b) {
        digitalWrite(txPin, LOW);     // 1
        delayMicroseconds(1400);
        digitalWrite(txPin, HIGH);
        delayMicroseconds(700);
      } else {
        digitalWrite(txPin, LOW);     // 0
        delayMicroseconds(700);
        digitalWrite(txPin, HIGH);
        delayMicroseconds(1400);
      }
    }
    digitalWrite(txPin, LOW);
    delayMicroseconds(25200);
  }
  displayTx = "Nice " + String(Code & 0xfff);
  TxDisplay();
}


//-----------------------------------------------------------------------------------------------------------------------------

//Прием

boolean CheckValue(unsigned int base, unsigned int value) {
  return ((value == base) || ((value > base) && ((value - base) < maxDelta)) || ((value < base) && ((base - value) < maxDelta)));
}

void grab() {
  state = digitalRead(rxPin);
  if (state == HIGH)
    lolen = micros() - prevtime;
  else
    hilen = micros() - prevtime;
  prevtime = micros();

  //AN-MOTORS
  if (state == HIGH)  {
    if (CheckValue(pulseAN, hilen) && CheckValue(pulseAN * 2, lolen)) {      // valid 1
      if (bcounter < 32)
        code1 = (code1 << 1) | 1;
      else if (bcounter < 64)
        code2 = (code2 << 1) | 1;
      bcounter++;
    }
    else if (CheckValue(pulseAN * 2, hilen) && CheckValue(pulseAN, lolen)) {  // valid 0
      if (bcounter < 32)
        code1 = (code1 << 1) | 0;
      else if (bcounter < 64)
        code2 = (code2 << 1) | 0;
      bcounter++;
    }
    else {
      bcounter = 0;
      code1 = 0;
      code2 = 0;
    }
    if (bcounter >= 65 && code2 != -1)  {
      if (CashTrigger == 1) {
        if (String(Cash2) != String(code2)) {
          Cash1 = code2;
          CashTrigger = 2;
        }
      } else {
        if (String(Cash1) != String(code2)) {
          Cash2 = code2;
          CashTrigger = 1;
        }
      }
      displayRx = String(code1, HEX) + " " + String(code2, HEX);
      displayRx.toUpperCase();
      cachView();
      bcounter = 0;
      code1 = 0;
      code2 = 0;
    }
  }


  //CAME
  if (state == LOW) {
    if (CheckValue(320, hilen) && CheckValue(640, lolen)) {        // valid 1
      cameCode = (cameCode << 1) | 1;
      cameCounter++;
    } else if (CheckValue(640, hilen) && CheckValue(320, lolen)) {   // valid 0
      cameCode = (cameCode << 1) | 0;
      cameCounter++;
    } else {
      cameCounter = 0;
      cameCode = 0;
    }
  }
  else if (cameCounter == 12 && lolen > 1000 && (cameCode & 0xfff) != 0xfff) {
    if (cashCameTrigger == 1) {
      if (String(cashCame2) != String(cameCode & 0xfff)) {
        cashCame1 = cameCode & 0xfff;
        cashCameTrigger = 2;
      }
    } else {
      if (String(cashCame1) != String(cameCode & 0xfff)) {
        cashCame2 = cameCode & 0xfff;
        cashCameTrigger = 1;
      }
    }
    displayRx = "Came " + String(cameCode & 0xfff);
    cachView();
    cameCounter = 0;
    cameCode = 0;
  }


  //NICE
  if (state == LOW) {
    if (CheckValue(700, hilen) && CheckValue(1400, lolen)) {        // valid 1
      niceCode = (niceCode << 1) | 1;
      niceCounter++;
    }
    else if (CheckValue(1400, hilen) && CheckValue(700, lolen)) {   // valid 0
      niceCode = (niceCode << 1) | 0;
      niceCounter++;
    }
    else {
      niceCounter = 0;
      niceCode = 0;
    }
  }
  else if (niceCounter == 12 && lolen > 2000 && (niceCode & 0xfff) != 0xfff) {
    if (cashNiceTrigger == 1) {
      if (String(cashNice2) != String(niceCode & 0xfff)) {
        cashNice1 = niceCode & 0xfff;
        cashNiceTrigger = 2;
      }
    } else {
      if (String(cashNice1) != String(niceCode & 0xfff)) {
        cashNice2 = niceCode & 0xfff;
        cashNiceTrigger = 1;
      }
    }
    displayRx = "Nice " + String(niceCode & 0xfff);
    cachView();
    niceCounter = 0;
    niceCode = 0;
  }


}

//-----------------------------------------------------------------------------------------------------------------------------


//ИНДИКАЦИЯ

void cachView() {
  if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
    digitalWrite(ledCach1, HIGH);
  } else {
    EEPROM.get(0, Cash1);
    EEPROM.get(20, cashCame1);
    EEPROM.get(40, cashNice1);
    if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
      digitalWrite(ledCach1, HIGH);
    } else {
      digitalWrite(ledCach1, LOW);
      if (clickCash == 1) {
        displayTx = "No cache data 1";
      }
    }
  }
  if (Cash2 != 0 || cashCame2 != 0 || cashNice2 != 0) {
    digitalWrite(ledCach1, HIGH);
    digitalWrite(ledCach2, HIGH);
  } else {
    EEPROM.get(10, Cash2);
    EEPROM.get(30, cashCame2);
    EEPROM.get(50, cashNice2);
    if (Cash2 != 0 || cashCame2 != 0 || cashNice2 != 0) {
      digitalWrite(ledCach1, HIGH);
      digitalWrite(ledCach2, HIGH);
    } else {
      digitalWrite(ledCach2, LOW);
      if (clickCash == 2) {
        displayTx = "No cache data 2";
      }
    }
  }
}

void RxDisplay() {
  if (displayRx != "") {
    clearDisplay();
    oled.print("Rx: ");
    oled.println(displayRx);
    bipOne();
    displayRx = "";
  }
}

void TxDisplay() {
  if (displayTx != "") {
    clearDisplay();
    if (displayTx == "No cache data 1" || displayTx == "No cache data 2") {
      bipLong(true);
      oled.print("Ms: ");
    } else {
      oled.print("Tx: ");
    }
    oled.println(displayTx);
    displayTx = "";
  }
}

void clearDisplay() {
  if (displayClear) {
    oled.clear();
    displayClear = false;
  }
}

void bipOne() {
  digitalWrite(bip, HIGH);
  delay(150);
  digitalWrite(bip, LOW);
}

void bipTwo() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(bip, HIGH);
    delay(100);
    digitalWrite(bip, LOW);
    delay(100);
  }
}

void bipLong(boolean led) {
  if (led == true) {
    digitalWrite(ledJammer, HIGH);
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, HIGH);
  }
  digitalWrite(bip, HIGH);
  delay(400);
  digitalWrite(bip, LOW);
  if (led == true) {
    digitalWrite(ledJammer, LOW);
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, LOW);
  }
}

long readVcc() {
#if defined(__AVR_ATmega32U4__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;
  result = (1.098 * 1023 * 1000 / result) - 3150;   // 1.098 - калибровочный коэффициент для более точной индикации заряда аккумулятора
  if (result > 999 || result < 1) {
    if (result > 999) {
      return 100;
    } else {
      return 0;
    }
  } else {
    return result * 0.1;
  }
}

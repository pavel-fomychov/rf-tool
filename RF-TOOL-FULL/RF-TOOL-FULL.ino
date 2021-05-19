#include <EEPROM.h>
#include "OneButton.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
SSD1306AsciiWire oled;
#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

#define pulseAN 400
#define rxPin 2                         // Приемник
#define rxOn 3                          // Включение приёмника
#define txPin 4                         // Передатчик
#define ledCach1 8                      // Индикатор кеша 1
#define ledCach2 6                      // Индикатор кеша 2
#define ledJammer 7                     // Индикатор глушилки
#define btsendPin1 A1                   // кнопка 1
#define btsendPin2 A2                   // кнопка 2
#define btsendPin3 11                   // кнопка 3
#define btsendPin4 10                   // кнопка 4
#define bip A0                          // Вибро
#define maxDelta 200                    // максимальное отклонение от длительности при приеме
boolean btnFlag3 = 1;                   // флаг для кнопка 3
boolean btnFlag4 = 1;                   // флаг для кнопка 4
volatile unsigned int staticMode = 0;                     // номер режима staticMode
volatile unsigned int switchMode = 0;                     // номер режима switchMode
OneButton button1(btsendPin1, false);   // вызов функции отслеживания кнопка 1
OneButton button2(btsendPin2, false);   // вызов функции отслеживания кнопка 2
OneButton button3(btsendPin3, false);   // вызов функции отслеживания кнопка 3
OneButton button4(btsendPin4, false);   // вызов функции отслеживания кнопка 4

volatile unsigned long prevtime;
volatile unsigned int lolen, hilen, state;
int clickCash;

//AN Motors
volatile static byte bcounter = 0;      // количество принятых битов
volatile static long code1 = 0;         // зашифрованная часть
volatile static long code2 = 0;         // фиксированная часть
volatile long c1 = 0;                   // переменная для отправки
volatile long c2 = 0;                   // переменная для отправки
volatile long Cash1Rand = 0;            // кеш переменная для отправки
volatile long Cash2Rand = 0;            // кеш переменная для отправки
volatile long Cash1 = 0;                // кеш переменная для отправки
volatile long Cash2 = 0;                // кеш переменная для отправки
int CashTrigger = 1;                    // переключение между перемеными кеша
//CAME
volatile static byte cameCounter = 0;   // сохраненое количество бит
volatile static long cameCode = 0;      // код Came
volatile static long cameCode12 = 0;    // код Came 12bit
volatile static long cameCode24 = 0;    // код Came 24bit
volatile long cashCame1 = 0;            // кеш переменная для отправки
volatile long cashCame2 = 0;            // кеш переменная для отправки
int cashCameTrigger = 1;                // переключение между перемеными кеша
//NICE
volatile static byte niceCounter = 0;   // сохраненое количество бит
volatile static long niceCode = 0;      // код Nice
volatile static long niceCode12 = 0;    // код Nice 12bit
volatile static long niceCode24 = 0;    // код Nice 24bit
volatile long cashNice1 = 0;            // кеш переменная для отправки
volatile long cashNice2 = 0;            // кеш переменная для отправки
int cashNiceTrigger = 1;                // переключение между перемеными кеша
//SAVE-MODE
volatile long Cash1SaveMode = 0;        // кеш переменная SaveMode
volatile long Cash2SaveMode = 0;        // кеш переменная SaveMode
//RC-SWITCH
long rcCode1 = 0;                       // кеш переменная для отправки
long rcCode2 = 0;                       // кеш переменная для отправки
long rcCode3 = 0;                       // кеш переменная для отправки
long rcCode4 = 0;                       // кеш переменная для отправки
int rcCounter1 = 0;                     // сохраненое количество бит
int rcCounter2 = 0;                     // сохраненое количество бит
int rcCounter3 = 0;                     // сохраненое количество бит
int rcCounter4 = 0;                     // сохраненое количество бит
long rcDelay1 = 0;                      // длительность периода
long rcDelay2 = 0;                      // длительность периода
long rcDelay3 = 0;                      // длительность периода
long rcDelay4 = 0;                      // длительность периода
long rcTrigger = 1;                     // переключение между перемеными кеша

//DISPLAY
String displayTx = "";                  // кеш дисплея передача
String displayRx = "";                  // кеш дисплея прием
boolean displayClear = true;            // первичная очистка дисплея
int current_page = 0;
int current_cell = 0;
int count_cell = 69;
int count_page = 0;

unsigned long voltage = 0;              // процент заряда АКБ

void setup() {
  //Serial.begin(9600);
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
  pinMode(btsendPin3, INPUT);
  pinMode(btsendPin4, INPUT);

  if (digitalRead(btsendPin3) == HIGH) {
    mySwitch.enableReceive(0);
    mySwitch.enableTransmit(4);
    mySwitch.setRepeatTransmit(4);
    switchMode = 1;
  } else if (digitalRead(btsendPin4) == HIGH) {
    attachInterrupt(0, grab, CHANGE);       // Перехват пакетов ( 1 для Pro Micro | 0 для Uno, Nano, Pro Mini )
    randomSeed(analogRead(0));              // Генерация случайного числа для AN-Motors

    button1.attachClick(click1);
    button2.attachClick(click2);
    button2.attachLongPressStart(longPressStart2);
    button3.attachClick(click3);
    button3.attachDoubleClick(doubleclick3);
    button3.attachLongPressStart(longPressStart3);
    button4.attachClick(click4);
    button4.attachDoubleClick(doubleclick4);
    button4.attachLongPressStart(longPressStart4);

    switchMode = 2;
  } else {
    cachView();                             // Индикатор кеша, запрос кеша из EEPROM
    attachInterrupt(0, grab, CHANGE);       // Перехват пакетов (1 для Pro Micro, 0 для Uno, Nano)
    randomSeed(analogRead(0));              // Генерация случайного числа для AN-Motors

    button1.attachClick(click1);
    button1.attachDoubleClick(doubleclick1);
    button1.attachLongPressStart(longPressStart1);
    button2.attachClick(click2);
    button2.attachDoubleClick(doubleclick2);
    button2.attachLongPressStart(longPressStart2);
    button3.attachClick(click3);
    button3.attachDoubleClick(doubleclick3);
    button3.attachLongPressStart(longPressStart3);
    button4.attachClick(click4);
    button4.attachDoubleClick(doubleclick4);
    button4.attachLongPressStart(longPressStart4);
  }

  oled.begin(&Adafruit128x64, 0x3C);
  oled.clear();
  oled.setFont(Arial14);

  unsigned long lasttime = 0;
  while (true) {
    if (lasttime < millis()) {
      voltage = readVcc();                    // Статус батареи
      if (switchMode == 1) {
        oled.setCursor(5, 2);
        oled.println("RC-SWITCH  MODE");
      } else if (switchMode == 2) {
        oled.setCursor(3, 2);
        oled.println("AUTO-SAVE  MODE");
      } else {
        oled.setCursor(29, 1);
        oled.println("RF - TOOLS");
        oled.setCursor(17, 10);
        oled.println("READY TO USE");
      }
      oled.setCursor(28, 20);
      oled.print("battery: ");
      oled.print(voltage);
      oled.println("%  ");
      lasttime = millis() + 60000;
      if (switchMode == 1) delay(1000);
      if (switchMode == 2) delay(2000);
    }
    if (analogRead(btsendPin1) > 550 || analogRead(btsendPin2) > 550 || digitalRead(btsendPin3) == HIGH || digitalRead(btsendPin4) == HIGH || switchMode == 2 || displayRx != "" || mySwitch.available() ) {
      oled.setFont(font5x7);
      oled.setCursor(0, 0);
      if (switchMode == 2) {
        oled.clear();
        getCodeEEPROM();
      } else {
        oled.setScrollMode(SCROLL_MODE_AUTO);
      }
      break;
    }
  }

}

void loop() {
  //Отслеживание нажатия кнопок
  if (switchMode == 1) {
    rcSwitch();
    SWbutton1();
    SWbutton2();
    SWbutton3();
    SWbutton4();
  } else {
    button1.tick();
    button2.tick();
    button3.tick();
    button4.tick();
  }
  //Вывод на дисплей принятых данных
  RxDisplay();
}

//Отправка кода из кеша 1
void click1() {
  if (switchMode == 2) {
    getCodeEEPROM();

    int num_cell = 0;
    long eeprom_val_1 = 0;
    long eeprom_val_2 = 0;
    num_cell = current_cell * 8 + 100;
    EEPROM.get(num_cell, eeprom_val_1);
    num_cell = num_cell + 4;
    EEPROM.get(num_cell, eeprom_val_2);

    if (eeprom_val_2 != 0 && eeprom_val_2 != -1) {
      digitalWrite(rxOn, LOW);                // Выкл перехват
      digitalWrite(ledCach1, HIGH);
      if (eeprom_val_1 == 5012) SendCame(eeprom_val_2, false);
      else if (eeprom_val_1 == 5024) SendCame(eeprom_val_2, true);
      else if (eeprom_val_1 == 6012) SendNice(eeprom_val_2, false);
      else if (eeprom_val_1 == 6024) SendNice(eeprom_val_2, true);
      else {
        c1 = 0x25250000 + random(0xffff);
        c2 = eeprom_val_2;
        SendANMotors(c1, c2);
      }
      bipOne();
      digitalWrite(ledCach1, LOW);
      digitalWrite(rxOn, HIGH);               // Вкл перехват
    } else {
      bipLong(true);
    }
  } else {
    clickCash = 1;
    cachView();
    if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
    }
    digitalWrite(rxOn, LOW);                // Выкл перехват
    if (Cash1 != 0) {
      if (staticMode == 1) {
        c1 = Cash1Rand;
      } else {
        c1 = 0x25250000 + random(0xffff);
      }
      c2 = Cash1;
      SendANMotors(c1, c2);
    }
    if (cashCame1 != 0) {
      if (staticMode == 1) {
        SendCame(cashCame1, true);
      } else {
        SendCame(cashCame1, false);
      }
    }
    if (cashNice1 != 0) {
      if (staticMode == 1) {
        SendNice(cashNice1, true);
      } else {
        SendNice(cashNice1, false);
      }
    }
    TxDisplay();
    digitalWrite(rxOn, HIGH);               // Вкл перехват
    if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
      bipOne();
    }
    cachView();
  }
}

//Отправка кода из кеша 2
void doubleclick1() {
  clickCash = 2;
  cachView();
  if (Cash2 != 0 || cashCame2 != 0 || cashNice2 != 0) {
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, HIGH);
  }
  digitalWrite(rxOn, LOW);                // Выкл перехват
  if (Cash2 != 0) {
    c1 = 0x25250000 + random(0xffff);
    if (staticMode == 1) {
      c1 = Cash2Rand;
    } else {
      c1 = 0x25250000 + random(0xffff);
    }
    c2 = Cash2;
    SendANMotors(c1, c2);
  }
  if (cashCame2 != 0) {
    if (staticMode == 1) {
      SendCame(cashCame2, true);
    } else {
      SendCame(cashCame2, false);
    }
  }
  if (cashNice2 != 0) {
    if (staticMode == 1) {
      SendNice(cashNice2, true);
    } else {
      SendNice(cashNice2, false);
    }
  }
  TxDisplay();
  digitalWrite(rxOn, HIGH);               // Вкл перехват
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
  if (Cash1Rand != 0) {
    EEPROM.put(60, Cash1Rand);
    checkCash = 1;
  }
  if (Cash2Rand != 0) {
    EEPROM.put(70, Cash2Rand);
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
  if (switchMode == 2) {
    clearCodeEEPROM(current_cell, 0);
    getCodeEEPROM();
  } else {
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
}

//StaticMode
void doubleclick2() {
  digitalWrite(ledJammer, HIGH);
  digitalWrite(ledCach1, HIGH);
  digitalWrite(ledCach2, HIGH);
  clearDisplay();
  if (staticMode == 0) {
    staticMode = 1;
    oled.println("Ms: Static/24 ON");
    bipTwo();
  } else {
    staticMode = 0;
    oled.println("Ms: Static/24 OFF");
    bipTwo();
  }
  digitalWrite(ledJammer, LOW);
  cachView();
}

//Очистка кеша
void longPressStart2() {
  if (switchMode == 2) {
    clearCodeEEPROM(0, 1);
    current_cell = 0;
    current_page = 0;
    getCodeEEPROM();
  } else {
    Cash1 = 0;
    Cash2 = 0;
    Cash1Rand = 0;
    Cash2Rand = 0;
    cashCame1 = 0;
    cashCame2 = 0;
    cashNice1 = 0;
    cashNice2 = 0;
    EEPROM.put(0, Cash1);
    EEPROM.put(10, Cash2);
    EEPROM.put(60, Cash1Rand);
    EEPROM.put(70, Cash2Rand);
    EEPROM.put(20, cashCame1);
    EEPROM.put(30, cashCame2);
    EEPROM.put(40, cashNice1);
    EEPROM.put(50, cashNice2);
    clearDisplay();
    oled.println("Ms: Cache is cleared");
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, LOW);
    bipLong(false);
  }
}

//Отправка Came <-
void click3() {
  if (switchMode == 2) {
    current_cell--;
    getCodeEEPROM();
    bipOne();
  } else {
    digitalWrite(rxOn, LOW);                // Выкл перехват
    if (cashCame1 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      cashCame1++;
      cashCame2++;
      if (staticMode == 1) {
        SendCame(cashCame1, true);
      } else {
        SendCame(cashCame1, false);
      }
      bipOne();
    } else {
      clearDisplay();
      oled.println("Ms: Hold to start");
      bipLong(true);
    }
    digitalWrite(rxOn, HIGH);               // Вкл перехват
    cachView();
  }
}

//Отправка Came ->
void doubleclick3() {
  if (switchMode == 2) {
    current_page = (int)current_cell / 7;
    current_cell = (current_page - 1) * 7;
    getCodeEEPROM();
    bipTwo();
  } else {
    digitalWrite(rxOn, LOW);                // Выкл перехват
    if (cashCame1 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      cashCame1--;
      cashCame2--;
      if (staticMode == 1) {
        SendCame(cashCame1, true);
      } else {
        SendCame(cashCame1, false);
      }
      bipTwo();
    } else {
      clearDisplay();
      oled.println("Ms: Hold to start");
      bipLong(true);
    }
    digitalWrite(rxOn, HIGH);               // Вкл перехват
    cachView();
  }
}

//Перебор кодов Came
void longPressStart3() {
  if (switchMode == 2) {
    current_cell = 0;
    getCodeEEPROM();
    bipLong(false);
  } else {
    digitalWrite(ledJammer, HIGH);
    digitalWrite(ledCach1, HIGH);
    digitalWrite(ledCach2, HIGH);
    digitalWrite(rxOn, LOW);                // Выкл перехват
    bipOne();
    oled.clear();
    for (int c = 4095; c >= 0; c--) {
      if (digitalRead(btsendPin3) == HIGH && btnFlag3 == 0) {
        digitalWrite(ledJammer, LOW);
        cashCame1 = c + 1;
        cashCame2 = c;
        btnFlag3 = 1;
        cachView();
        break;
      }
      if (digitalRead(btsendPin3) == LOW && btnFlag3 == 1) {
        btnFlag3 = 0;
      }
      if (staticMode == 1) {
        SendCame(c, true);
      } else {
        SendCame(c, false);
      }
      if (c == 0) {
        bipLong(false);
        digitalWrite(ledJammer, LOW);
        btnFlag3 = 1;
        cachView();
      }
    }
    digitalWrite(rxOn, HIGH);               // Вкл перехват
  }
}

//Отправка Nice <-
void click4() {
  if (switchMode == 2) {
    current_cell++;
    getCodeEEPROM();
    bipOne();
  } else {
    digitalWrite(rxOn, LOW);                // Выкл перехват
    if (cashNice1 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      cashNice1++;
      cashNice2++;
      if (staticMode == 1) {
        SendNice(cashNice1, true);
      } else {
        SendNice(cashNice1, false);
      }
      bipOne();
    } else {
      clearDisplay();
      oled.println("Ms: Hold to start");
      bipLong(true);
    }
    digitalWrite(rxOn, HIGH);               // Вкл перехват
    cachView();
  }
}

//Отправка Nice ->
void doubleclick4() {
  if (switchMode == 2) {
    current_page = (int)current_cell / 7;
    current_cell = (current_page + 1) * 7;
    getCodeEEPROM();
    bipTwo();
  } else {
    digitalWrite(rxOn, LOW);                // Выкл перехват
    if (cashNice1 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      cashNice1--;
      cashNice2--;
      if (staticMode == 1) {
        SendNice(cashNice1, true);
      } else {
        SendNice(cashNice1, false);
      }
      bipTwo();
    } else {
      clearDisplay();
      oled.println("Ms: Hold to start");
      bipLong(true);
    }
    digitalWrite(rxOn, HIGH);               // Вкл перехват
    cachView();
  }
}

//Перебор кодов Nice
void longPressStart4() {
  if (switchMode == 2) {
    current_cell = count_cell - 1;
    current_cell = int(count_cell / 7) * 7;
    getCodeEEPROM();
    bipLong(false);
  } else {
    digitalWrite(ledJammer, HIGH);
    digitalWrite(ledCach1, HIGH);
    digitalWrite(ledCach2, HIGH);
    digitalWrite(rxOn, LOW);                // Выкл перехват
    bipOne();
    oled.clear();
    for (int n = 4095; n >= 0; n--) {
      if (digitalRead(btsendPin4) == HIGH && btnFlag4 == 0) {
        digitalWrite(ledJammer, LOW);
        cashNice1 = n + 1;
        cashNice2 = n;
        btnFlag4 = 1;
        cachView();
        break;
      }
      if (digitalRead(btsendPin4) == LOW && btnFlag4 == 1) {
        btnFlag4 = 0;
      }
      if (staticMode == 1) {
        SendNice(n, true);
      } else {
        SendNice(n, false);
      }
      if (n == 0) {
        bipLong(false);
        digitalWrite(ledJammer, LOW);
        btnFlag4 = 1;
        cachView();
      }
    }
    digitalWrite(rxOn, HIGH);               // Вкл перехват
  }
}

//Switch Mode кнопка 1
void SWbutton1() {
  if (analogRead(btsendPin1) > 550) {
    if (rcCode1 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      digitalWrite(rxOn, LOW);                // Выкл перехват
      mySwitch.setPulseLength(rcDelay1);
      mySwitch.send(rcCode1, rcCounter1);
      displayTx = String(rcCode1) + " " + String(rcCounter1) + "bit";
      TxDisplay();
      digitalWrite(rxOn, HIGH);               // Вкл перехват
      digitalWrite(ledCach2, LOW);
    } else {
      displayTx = "No cache data 1";
      TxDisplay();
    }
  }
}
//Switch Mode кнопка 2
void SWbutton2() {
  if (analogRead(btsendPin2) > 550) {
    if (rcCode2 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      digitalWrite(rxOn, LOW);                // Выкл перехват
      mySwitch.setPulseLength(rcDelay2);
      mySwitch.send(rcCode2, rcCounter2);
      displayTx = String(rcCode2) + " " + String(rcCounter2) + "bit";
      TxDisplay();
      digitalWrite(rxOn, HIGH);               // Вкл перехват
      digitalWrite(ledCach2, LOW);
    } else {
      displayTx = "No cache data 2";
      TxDisplay();
    }
  }
}
//Switch Mode кнопка 3
void SWbutton3() {
  if (digitalRead(btsendPin3) == HIGH && btnFlag3 == 0) {
    if (rcCode3 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      digitalWrite(rxOn, LOW);                // Выкл перехват
      mySwitch.setPulseLength(rcDelay3);
      mySwitch.send(rcCode3, rcCounter3);
      displayTx = String(rcCode3) + " " + String(rcCounter3) + "bit";
      TxDisplay();
      digitalWrite(rxOn, HIGH);               // Вкл перехват
      digitalWrite(ledCach2, LOW);
    } else {
      displayTx = "No cache data 3";
      TxDisplay();
    }
  }
}
//Switch Mode кнопка 4
void SWbutton4() {
  if (digitalRead(btsendPin4) == HIGH && btnFlag4 == 0) {
    if (rcCode4 != 0) {
      digitalWrite(ledCach1, LOW);
      digitalWrite(ledCach2, HIGH);
      digitalWrite(rxOn, LOW);                // Выкл перехват
      mySwitch.setPulseLength(rcDelay4);
      mySwitch.send(rcCode4, rcCounter4);
      displayTx = String(rcCode4) + " " + String(rcCounter4) + "bit";
      TxDisplay();
      digitalWrite(rxOn, HIGH);               // Вкл перехват
      digitalWrite(ledCach2, LOW);
    } else {
      displayTx = "No cache data 4";
      TxDisplay();
    }
  }
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

  if (switchMode == 2) {
    displayTx = "";
  } else {
    displayTx = String(c1, HEX) + " " + String(c2, HEX);
    displayTx.toUpperCase();
    TxDisplay();
  }
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
void SendCame(long Code, bool bit24) {
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
        if (bit24) {
          digitalWrite(txPin, LOW);
          delayMicroseconds(640);
          digitalWrite(txPin, HIGH);
          delayMicroseconds(320);
        }
      } else {
        digitalWrite(txPin, LOW);     // 0
        delayMicroseconds(320);
        digitalWrite(txPin, HIGH);
        delayMicroseconds(640);
        if (bit24) {
          digitalWrite(txPin, LOW);
          delayMicroseconds(320);
          digitalWrite(txPin, HIGH);
          delayMicroseconds(640);
        }
      }
    }
    digitalWrite(txPin, LOW);
    if (bit24)
      delayMicroseconds(23040);
    else
      delayMicroseconds(11520);
  }

  if (switchMode == 2) {
    displayTx = "";
  } else {
    if (bit24) {
      displayTx = "Came 24bit " + String(Code & 0xfff);
    } else {
      displayTx = "Came 12bit " + String(Code & 0xfff);
    }
    TxDisplay();
  }
}

//NICE
void SendNice(long Code, bool bit24) {
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
        if (bit24) {
          digitalWrite(txPin, LOW);
          delayMicroseconds(1400);
          digitalWrite(txPin, HIGH);
          delayMicroseconds(700);
        }
      } else {
        digitalWrite(txPin, LOW);     // 0
        delayMicroseconds(700);
        digitalWrite(txPin, HIGH);
        delayMicroseconds(1400);
        if (bit24) {
          digitalWrite(txPin, LOW);
          delayMicroseconds(700);
          digitalWrite(txPin, HIGH);
          delayMicroseconds(1400);
        }
      }
    }
    digitalWrite(txPin, LOW);
    if (bit24)
      delayMicroseconds(50400);
    else
      delayMicroseconds(25200);
  }

  if (switchMode == 2) {
    displayTx = "";
  } else {
    if (bit24) {
      displayTx = "Nice 24bit " + String(Code & 0xfff);
    } else {
      displayTx = "Nice 12bit " + String(Code & 0xfff);
    }
    TxDisplay();
  }
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
      if (switchMode == 2) {
        Cash1SaveMode = code1;
        Cash2SaveMode = code2;
        displayRx = "SaveMode";
      } else {
        if (CashTrigger == 1) {
          if (String(Cash2) != String(code2)) {
            Cash1 = code2;
            Cash1Rand = code1;
            CashTrigger = 2;
          }
        } else {
          if (String(Cash1) != String(code2)) {
            Cash2 = code2;
            Cash2Rand = code1;
            CashTrigger = 1;
          }
        }

        if (String(Cash1) == String(code2)) {
          Cash1Rand = code1;
        }
        if (String(Cash2) == String(code2)) {
          Cash2Rand = code1;
        }

        displayRx = String(code1, HEX) + " " + String(code2, HEX);
        displayRx.toUpperCase();
        cachView();
      }

      bcounter = 0;
      code1 = 0;
      code2 = 0;
    }
  }

  //CAME
  if (state == LOW) {
    if (CheckValue(320, hilen) && CheckValue(640, lolen)) {        // valid 1
      cameCode12 = (cameCode12 << 1) | 1;
      cameCounter++;
      if (cameCounter % 2 == 0) {
        cameCode24 = (cameCode24 << 1) | 1;
      }
    }
    else if (CheckValue(640, hilen) && CheckValue(320, lolen)) {   // valid 0
      cameCode12 = (cameCode12 << 1) | 0;
      cameCounter++;
      if (cameCounter % 2 == 0) {
        cameCode24 = (cameCode24 << 1) | 0;
      }
    }
    else {
      cameCounter = 0;
      cameCode12 = 0;
      cameCode24 = 0;
      cameCode = 0;
    }
  }
  else if ((cameCounter == 12 || cameCounter == 24) && lolen > 1000 && (cameCode & 0xfff) != 0xfff) {
    if (switchMode == 2) {
      if (cameCounter == 12 || cameCounter == 13) {
        Cash1SaveMode = 5012; //Came 12bit
        Cash2SaveMode = cameCode12;
      } else {
        Cash1SaveMode = 5024; //Came 24bit
        Cash2SaveMode = cameCode24;
      }
      displayRx = "SaveMode";
    } else {
      if (cameCounter == 12 || cameCounter == 13) {
        cameCode = cameCode12;
        displayRx = "Came 12bit " + String(cameCode & 0xfff);
      } else {
        cameCode = cameCode24;
        displayRx = "Came 24bit " + String(cameCode & 0xfff);
      }

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
      cachView();
    }

    cameCounter = 0;
    cameCode12 = 0;
    cameCode24 = 0;
    cameCode = 0;
  }


  //NICE
  if (state == LOW) {
    if (CheckValue(700, hilen) && CheckValue(1400, lolen)) {        // valid 1
      niceCode12 = (niceCode12 << 1) | 1;
      niceCounter++;
      if (niceCounter % 2 == 0) {
        niceCode24 = (niceCode24 << 1) | 1;
      }
    }
    else if (CheckValue(1400, hilen) && CheckValue(700, lolen)) {   // valid 0
      niceCode12 = (niceCode12 << 1) | 0;
      niceCounter++;
      if (niceCounter % 2 == 0) {
        niceCode24 = (niceCode24 << 1) | 0;
      }
    }
    else {
      niceCounter = 0;
      niceCode12 = 0;
      niceCode24 = 0;
      niceCode = 0;
    }
  }
  else if ((niceCounter == 12 || niceCounter == 24) && lolen > 2000 && (niceCode & 0xfff) != 0xfff) {
    if (switchMode == 2) {
      if (niceCounter == 12 || niceCounter == 13) {
        Cash1SaveMode = 6012; //Nice 12bit
        Cash2SaveMode = niceCode12;
      } else {
        Cash1SaveMode = 6024; //Nice 24bit
        Cash2SaveMode = niceCode24;
      }
      displayRx = "SaveMode";
    } else {
      if (niceCounter == 12 || niceCounter == 13) {
        niceCode = niceCode12;
        displayRx = "Nice 12bit " + String(niceCode & 0xfff);
      } else {
        niceCode = niceCode24;
        displayRx = "Nice 24bit " + String(niceCode & 0xfff);
      }

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
      cachView();
    }

    niceCounter = 0;
    niceCode12 = 0;
    niceCode24 = 0;
    niceCode = 0;
  }


}


void rcSwitch() {
  if (digitalRead(btsendPin3) == LOW && digitalRead(btsendPin4) == LOW) {
    btnFlag3 = 0;
    btnFlag4 = 0;
  }
  if (mySwitch.available()) {
    long valueSwitch = mySwitch.getReceivedValue();
    long lengthSwitch = mySwitch.getReceivedBitlength();
    long delaySwitch = mySwitch.getReceivedDelay();
    if (valueSwitch != 0) {
      displayRx = String(valueSwitch) + " " + String(lengthSwitch) + "bit";
      if (rcTrigger == 1) {
        if (rcCode2 != valueSwitch && rcCode3 != valueSwitch && rcCode4 != valueSwitch) {
          rcCode1 = valueSwitch;
          rcCounter1 = lengthSwitch;
          rcDelay1 = delaySwitch;
          rcTrigger = 2;
        }
      }
      if (rcTrigger == 2) {
        if (rcCode1 != valueSwitch && rcCode3 != valueSwitch && rcCode4 != valueSwitch) {
          rcCode2 = valueSwitch;
          rcCounter2 = lengthSwitch;
          rcDelay2 = delaySwitch;
          rcTrigger = 3;
        }
      }
      if (rcTrigger == 3) {
        if (rcCode1 != valueSwitch && rcCode2 != valueSwitch && rcCode4 != valueSwitch) {
          rcCode3 = valueSwitch;
          rcCounter3 = lengthSwitch;
          rcDelay3 = delaySwitch;
          rcTrigger = 4;
        }
      }
      if (rcTrigger == 4) {
        if (rcCode1 != valueSwitch && rcCode2 != valueSwitch && rcCode3 != valueSwitch) {
          rcCode4 = valueSwitch;
          rcCounter4 = lengthSwitch;
          rcDelay4 = delaySwitch;
          rcTrigger = 1;
        }
      }
    }
    mySwitch.resetAvailable();
  }
  if (rcCode1 > 0) {
    digitalWrite(ledCach1, HIGH);
    digitalWrite(ledCach2, HIGH);
  }
}


void getCodeEEPROM() {
  oled.setCursor(0, 0);

  if (current_cell >= count_cell) current_cell = 0;
  if (current_cell < 0) current_cell = count_cell - 1;

  count_page = (int)count_cell / 7;
  current_page = (int)current_cell / 7;
  if (current_page > count_page) current_page = 0;
  if (current_page < 0) current_page = count_page;

  if (count_cell > 0) {
    long eeprom_val_1 = 0;
    long eeprom_val_2 = 0;
    int num = (int)current_page * 7;
    int cell = current_page * 8 * 7 + 100;
    int cell_before = cell + 8 * 7;

    while (cell < cell_before) {
      if (num < count_cell) {
        if (num == current_cell) oled.setInvertMode(1);

        EEPROM.get(cell, eeprom_val_1);
        cell = cell + 4;
        EEPROM.get(cell, eeprom_val_2);
        cell = cell + 4;

        oled.print(" ");
        if (num < 9) oled.print(0);
        oled.print(num + 1);
        oled.print(": ");
        if (eeprom_val_2 != 0 && eeprom_val_2 != -1) {
          if (eeprom_val_1 == 5012) oled.print("Came 12bit");
          else if (eeprom_val_1 == 5024) oled.print("Came 24bit");
          else if (eeprom_val_1 == 6012) oled.print("Nice 12bit");
          else if (eeprom_val_1 == 6024) oled.print("Nice 24bit");
          else oled.print("KeeLog");
          oled.print(" ");
          if (eeprom_val_1 > 5010 && eeprom_val_1 < 6030) oled.print(eeprom_val_2 & 0xfff);
          else oled.print(eeprom_val_2, HEX);
          oled.println("               ");
        } else {
          oled.println("[-empty-cell-]    ");
        }

        if (num == current_cell) oled.setInvertMode(0);
      } else {
        cell = cell + 8;
        oled.println("                      ");
      }
      num++;
    }
  }

  oled.print("  < Page: ");
  oled.print(current_page + 1);
  oled.print(" of ");
  oled.print(count_page + 1);
  oled.println(" >   ");
}

void saveCodeEEPROM(volatile long val_1, volatile long val_2) {
  int num_cell = 0;
  int val_cell = 0;
  int cell_start = 0;
  long eeprom_val = 0;
  int flagExists = 0;
  EEPROM.get(90, val_cell);

  cell_start = 104;
  while (cell_start < count_cell * 8 + 100) {
    EEPROM.get(cell_start, eeprom_val);
    if (eeprom_val == 0 || eeprom_val == -1) {
      val_cell = (cell_start - 100) / 8 ;
      break;
    }
    cell_start = cell_start + 8;
  }

  if (val_cell < 0 || val_cell >= count_cell) val_cell = 0;
  current_cell = val_cell;

  cell_start = 104;
  while (cell_start < count_cell * 8 + 100) {
    EEPROM.get(cell_start, eeprom_val);
    if (eeprom_val == val_2) {
      current_cell = (cell_start - 100) / 8 ;
      flagExists = 1;
      break;
    }
    cell_start = cell_start + 8;
  }

  if (flagExists == 0) {
    digitalWrite(ledJammer, HIGH);
    digitalWrite(ledCach1, HIGH);
    digitalWrite(ledCach2, HIGH);
    EEPROM.put(90, val_cell + 1);
    num_cell = val_cell * 8 + 100;
    EEPROM.put(num_cell, val_1);
    num_cell = num_cell + 4;
    EEPROM.put(num_cell, val_2);
    bipOne();
  } else {
    digitalWrite(ledCach2, HIGH);
    bipLong(false);
  }
  digitalWrite(ledJammer, LOW);
  digitalWrite(ledCach1, LOW);
  digitalWrite(ledCach2, LOW);
}

void clearCodeEEPROM(int num_cell, int all_clear) {
  digitalWrite(ledJammer, HIGH);
  long eeprom_val = 0;
  if (all_clear == 1) {
    current_cell = 0;
    EEPROM.put(90, eeprom_val);
    int cell_start = 100;
    while (cell_start < count_cell * 8 + 104) {
      EEPROM.put(cell_start, eeprom_val);
      cell_start = cell_start + 4;
    }
    bipLong(false);
  } else {
    EEPROM.put(90, current_cell);
    num_cell = current_cell * 8 + 100;
    EEPROM.put(num_cell, eeprom_val);
    num_cell = num_cell + 4;
    EEPROM.put(num_cell, eeprom_val);
    bipOne();
  }
  digitalWrite(ledJammer, LOW);
}


//-----------------------------------------------------------------------------------------------------------------------------

//Индикация

void cachView() {
  if (Cash1 != 0 || cashCame1 != 0 || cashNice1 != 0) {
    digitalWrite(ledCach1, HIGH);
  } else {
    EEPROM.get(0, Cash1);
    EEPROM.get(20, cashCame1);
    EEPROM.get(40, cashNice1);
    EEPROM.get(60, Cash1Rand);
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
    EEPROM.get(70, Cash2Rand);
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
    if (switchMode == 2) {
      saveCodeEEPROM(Cash1SaveMode, Cash2SaveMode);
      getCodeEEPROM();
    } else {
      clearDisplay();
      oled.print("Rx: ");
      oled.println(displayRx);
      bipOne();
    }
    displayRx = "";
  }
}

void TxDisplay() {
  if (displayTx != "") {
    clearDisplay();
    if (displayTx == "No cache data 1" || displayTx == "No cache data 2" || displayTx == "No cache data 3" || displayTx == "No cache data 4") {
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
  if (switchMode == 1) {
    digitalWrite(ledJammer, HIGH);
    digitalWrite(ledCach1, HIGH);
    digitalWrite(ledCach2, HIGH);
  }
  digitalWrite(bip, HIGH);
  delay(150);
  digitalWrite(bip, LOW);
  if (switchMode == 1) {
    digitalWrite(ledJammer, LOW);
    digitalWrite(ledCach1, LOW);
    digitalWrite(ledCach2, LOW);
  }
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
  result = (1.098 * 1023 * 1000 / result) - 3150;
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

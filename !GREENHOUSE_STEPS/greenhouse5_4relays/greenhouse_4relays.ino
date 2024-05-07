// эмуляция проекта тут 
// https://wokwi.com/projects/389450801370869761


/*  /////////////// пояснения по прошивке ////////////////////
  // использование встроенного OTA update
  // зайди на адрес x.x.x.x/ota_update для открытия страницы обновления
  // Скетч/Экспорт бинарного файла (для получения файла прошивки)
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  // х.х.х.х адрес еспшки в сети, его ты указываешь ниже.
  // он будет что то типа staticIP(192, 168, 3, 201); но вместо числа 3 надо указать свое
  // нажимаем Win + R
  // пишем cmd жмем Enter
  // открывается черное окно, пишем ipconfig жмем Enter
  // появляется куча информации о сети, надо там найти:
  // "IPv4 адрес. . . . . . . . . . : 192.168.10.ххх "
  // вместо ххх у вас будет айпишник вашего компьютера \ ноутбука
  // вместо третьего числа "10" будет ваша подсеть ( может быть 1, может 100, какое угодно число) укажите его 
  // так же важно указать в настройках все остальные наборы чисел как у вас  вылезло на черном экране
  // не закрывайте это окно, а спуститесь ниже по скетчу, там где пять строк с IPAddress
  // и измените там все как у вас в этом черном окне
*/

#define STATIC_IP // закомментировать если подключаетесь к мобильной точке доступа на телефоне
// const char* ssid = "esp32";
// const char* password = "123456789000";
const char* ssid = "KIBERnet_11136";
const char* password = "#3Ac8G@7";

#ifdef STATIC_IP
//со статическим айпишничком
IPAddress staticIP(192, 168, 1, 23); // важно правильно указать третье число - подсеть, смотри пояснения выше
IPAddress gateway(192, 168, 1, 1);    // и тут изменить тройку на свою подсеть
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 1, 1);       // изменить тройку на свою подсеть
IPAddress dns2(8, 8, 8, 8);
#endif

// настройка реле
#define RELE1 14
#define RELE2 27
#define RELE3 26
#define RELE4 25
#define ON 0
#define OFF 1

// выбрать одно из двух
#include <WiFi.h>       // esp32
// #include <ESP8266WiFi.h>       // esp8266

#include <GyverNTP.h>
GyverNTP ntp(3);

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS); // для проверки файлов

//настройки, хранятся в памяти EEPROM
struct Settings {
  GPtime startTime1;
  GPtime stopTime1;
  GPtime startTime2;
  GPtime stopTime2;
  GPtime startTime3;
  GPtime stopTime3;
  GPtime startTime4;
  GPtime stopTime4;
  bool rele_1_isOn = 0;
  bool rele_2_isOn = 0;
  bool rele_3_isOn = 0;
  bool rele_4_isOn = 0;
};
Settings setting;


#include <EEPROM.h>
#include <EEManager.h>  // подключаем либу
EEManager  memory(setting); // передаём нашу переменную (фактически её адрес)

GPdate nowDate;
GPtime nowTime;
uint32_t startSeconds1 = 0;
uint32_t startSeconds2 = 0;
uint32_t startSeconds3 = 0;
uint32_t startSeconds4 = 0;
uint32_t stopSeconds1 = 0;
uint32_t stopSeconds2 = 0;
uint32_t stopSeconds3 = 0;
uint32_t stopSeconds4 = 0;


// поддержка wifi связи
void wifiSupport() {
  if (WiFi.status() != WL_CONNECTED) {
    // Подключаемся к Wi-Fi
    Serial.print("try conn to ");
    Serial.print(ssid);
    Serial.print(":");
    WiFi.mode(WIFI_STA);
#ifdef STATIC_IP
    if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false) {
      Serial.println("wifi config failed.");
      return;
    }
#endif

    WiFi.begin(ssid, password);
    uint8_t trycon = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if (trycon++ < 30) {
        Serial.print(".");
        delay(500);
      }
      else {
        Serial.print("no connection to Wifi. Esp restarts NOW!");
        delay(1000);
        ESP.restart();
      }
    }
    Serial.println("Connected. \nIP: ");

    // Выводим IP ESP32
    Serial.println(WiFi.localIP());
  }
}//wifiSupport()


//проверка NTP связи
void checkNTPstauts() {
  //проверим статус обновления ntp
  byte ntpErr = ntp.status();
  if (ntpErr) {
    Serial.print("ntp err ");
    Serial.println(ntpErr);
  }
  /* Код ошибок NTP
    // 0 - всё ок
    // 1 - не запущен UDP
    // 2 - не подключен WiFi
    // 3 - ошибка подключения к серверу
    // 4 - ошибка отправки пакета
    // 5 - таймаут ответа сервера
    // 6 - получен некорректный ответ сервера
  */
  if (!ntp.synced()) Serial.println("NTP not sync");
}//checkNTPstauts()


// конструктор WEB страницы
void build() {
  GP.BUILD_BEGIN(600);
  GP.THEME(GP_DARK);
  GP.PAGE_TITLE("Greenhouse");
  // GP.PAGE_TITLE("JRENQ HAYAT@ :-)"); // для армянского друга Сена
  GP.ONLINE_CHECK();

  //все обновляющиеся параметры на WEB странице надо указать тут
  GP.UPDATE("nowDate,nowTime,start1Time,stop1Time,rele1Indikator,start21Time,stop2Time,rele2Indikator,start2Time,stop2Time,rele3Indikator,start4Time,stop4Time4,rele4Indikator");
  GP.TITLE("Теплица 2024", "t1");
  // GP.TITLE("ՋՐԵՆՔ ՀԱՅԱԹԸ", "t1");// для армянского друга Сена

  GP.LABEL("Сегодня: "); GP.BREAK();
  GP.DATE("nowDate", nowDate);
  GP.TIME("nowTime", nowTime);     GP.BREAK();
  GP.BREAK();
  GP.BREAK();
  GP.LABEL("Включим реле1: ");
  GP.TIME("start1Time", setting.startTime1);     GP.BREAK();
  GP.LABEL("и выключим после: ");
  GP.TIME("stop1Time", setting.stopTime1);    GP.BREAK();
  GP.LABEL("Реле 1:");
  GP.LED_RED("rele1Indikator", setting.rele_1_isOn);
  GP.BREAK();
  GP.BUTTON("btn1", "Вкл / Откл");
  GP.BREAK();
  GP.BREAK();

  GP.LABEL("Включим реле2: ");
  GP.TIME("start2Time", setting.startTime2);     GP.BREAK();
  GP.LABEL("и выключим после: ");
  GP.TIME("stop2Time", setting.stopTime2);    GP.BREAK();
  GP.LABEL("Реле 2:");
  GP.LED_RED("rele2Indikator", setting.rele_2_isOn);
  GP.BREAK();
  GP.BUTTON("btn2", "Вкл / Откл");
  GP.BREAK();
  GP.BREAK();

  GP.LABEL("Включим реле3: ");
  GP.TIME("start3Time", setting.startTime3);     GP.BREAK();
  GP.LABEL("и выключим после: ");
  GP.TIME("stop3Time", setting.stopTime3);     GP.BREAK();
  GP.LABEL("Реле 3:");
  GP.LED_RED("rele3Indikator", setting.rele_3_isOn);
  GP.BREAK();
  GP.BUTTON("btn3", "Вкл / Откл");
  GP.BREAK();
  GP.BREAK();

  GP.LABEL("Включим реле4: ");
  GP.TIME("start4Time", setting.startTime4);     GP.BREAK();
  GP.LABEL("и выключим после: ");
  GP.TIME("stop4Time", setting.stopTime4);   GP.BREAK();
  GP.LABEL("Реле 4:");
  GP.LED_RED("rele4Indikator", setting.rele_4_isOn);
  GP.BREAK();
  GP.BUTTON("btn4", "Вкл / Откл");
  GP.BREAK();
  GP.BREAK();

  GP.BREAK();
  GP.BREAK();
  GP.BUILD_END();
}


void action() {
  // было обновление
  if (ui.update()) {
    ui.updateDate("nowDate", nowDate);
    ui.updateTime("nowTime", nowTime);
    ui.updateTime("start1Time", setting.startTime1);
    ui.updateTime("stop1Time", setting.stopTime1);
    ui.updateTime("start2Time", setting.startTime2);
    ui.updateTime("stop2Time", setting.stopTime2);
    ui.updateTime("start3Time", setting.startTime3);
    ui.updateTime("stop3Time", setting.stopTime3);
    ui.updateTime("start4Time", setting.startTime4);
    ui.updateTime("stop4Time", setting.stopTime4);

    ui.updateBool("rele1Indikator", setting.rele_1_isOn);
    ui.updateBool("rele2Indikator", setting.rele_2_isOn);
    ui.updateBool("rele3Indikator", setting.rele_3_isOn);
    ui.updateBool("rele4Indikator", setting.rele_4_isOn);

  }//ui.update()
  if (ui.click()) {
    if (ui.clickDate("nowDate", nowDate)) {
      //      Serial.print("Date: ");
      //      Serial.println(valDate.encode());
    }
    if (ui.clickTime("nowTime", nowTime)) {
      //      Serial.print("Time: ");
      //      Serial.println(valTime.encode());
    }

    // обновление времени запуска и отключения нагрузки1
    if (ui.clickTime("start1Time", setting.startTime1)) {
      startSeconds1 = setting.startTime1.hour * 60 * 60 + setting.startTime1.minute * 60 + setting.startTime1.second;
      memory.updateNow();
    }
    if (ui.clickTime("stop1Time", setting.stopTime1)) {
      stopSeconds1 = setting.stopTime1.hour * 60 * 60 + setting.stopTime1.minute * 60 + setting.stopTime1.second;
      memory.updateNow();
    }
    // обновление времени запуска и отключения нагрузки2
    if (ui.clickTime("start2Time", setting.startTime2)) {
      startSeconds2 = setting.startTime2.hour * 60 * 60 + setting.startTime2.minute * 60 + setting.startTime2.second;
      memory.updateNow();
    }
    if (ui.clickTime("stop2Time", setting.stopTime2)) {
      stopSeconds2 = setting.stopTime2.hour * 60 * 60 + setting.stopTime2.minute * 60 + setting.stopTime2.second;
      memory.updateNow();
    }
    // обновление времени запуска и отключения нагрузки3
    if (ui.clickTime("start3Time", setting.startTime3)) {
      startSeconds3 = setting.startTime3.hour * 60 * 60 + setting.startTime3.minute * 60 + setting.startTime3.second;
      memory.updateNow();
    }
    if (ui.clickTime("stop3Time", setting.stopTime3)) {
      stopSeconds3 = setting.stopTime3.hour * 60 * 60 + setting.stopTime3.minute * 60 + setting.stopTime3.second;
      memory.updateNow();
    }
    // обновление времени запуска и отключения нагрузки4
    if (ui.clickTime("start4Time", setting.startTime4)) {
      startSeconds4 = setting.startTime4.hour * 60 * 60 + setting.startTime4.minute * 60 + setting.startTime4.second;
      memory.updateNow();
    }
    if (ui.clickTime("stop4Time", setting.stopTime4)) {
      stopSeconds4 = setting.stopTime4.hour * 60 * 60 + setting.stopTime4.minute * 60 + setting.stopTime4.second;
      memory.updateNow();
    }




    if (ui.click("btn1")) {
      setting.rele_1_isOn = !setting.rele_1_isOn;
      if (setting.rele_1_isOn) digitalWrite(RELE1, ON);
      else digitalWrite(RELE1, OFF);
      memory.updateNow();
    }
    if (ui.click("btn2")) {
      setting.rele_2_isOn = !setting.rele_2_isOn;
      if (setting.rele_2_isOn) digitalWrite(RELE2, ON);
      else digitalWrite(RELE2, OFF);
      memory.updateNow();
    }
    if (ui.click("btn3")) {
      setting.rele_3_isOn = !setting.rele_3_isOn;
      if (setting.rele_3_isOn) digitalWrite(RELE3, ON);
      else digitalWrite(RELE3, OFF);
      memory.updateNow();
    }
    if (ui.click("btn4")) {
      setting.rele_4_isOn = !setting.rele_4_isOn;
      if (setting.rele_4_isOn) digitalWrite(RELE4, ON);
      else digitalWrite(RELE4, OFF);
      memory.updateNow();
    }
  }//ui.click()
}//action()


void setup() {
  Serial.begin(115200);
  wifiSupport();

  // подключаем конструктор и запускаем
  ui.attachBuild(build);
  ui.attach(action);
  ui.start();
  ui.enableOTA();   // без пароля
  //ui.enableOTA("admin", "pass");  // с паролем

  if (!LittleFS.begin()) Serial.println("FS Error");
  ui.downloadAuto(true);

  ntp.begin();
  ntp.setGMT(4); // часовой пояс. Для Москвы: 3
  ntp.setPeriod(600); // обновлять раз в 600 сек
  EEPROM.begin(100);  // выделить память (больше или равно размеру структуры данных)
  memory.begin(0, 'e'); // изменить букву в скобках на другую, чтобы восстановить настройки по умолчанию

  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  pinMode(RELE3, OUTPUT);
  pinMode(RELE4, OUTPUT);
  if (setting.rele_1_isOn)   digitalWrite(RELE1, ON);
  else  digitalWrite(RELE1, OFF);
  if (setting.rele_2_isOn)   digitalWrite(RELE2, ON);
  else  digitalWrite(RELE2, OFF);
  if (setting.rele_3_isOn)   digitalWrite(RELE3, ON);
  else  digitalWrite(RELE3, OFF);
  if (setting.rele_4_isOn)   digitalWrite(RELE4, ON);
  else  digitalWrite(RELE4, OFF);
}//setup()


void loop() {
  ui.tick();
  ntp.tick();
  memory.tick();

  // раз в 10 сек проверим стабильность сети
  static uint32_t ms1 = 0;
  if (millis() - ms1 >= 10000) {
    ms1 = millis();
    wifiSupport();
    checkNTPstauts();
  }//ms

  // раз в 1 сек проверяем релюшки, переключаем их
  static uint32_t ms2 = 0;
  if (millis() - ms2 >= 1000) {
    ms2 = millis();

    // отдаем текущую дату и время переменным в веб интерфейс
    nowTime.set(ntp.hour(), ntp.minute(), ntp.second());
    nowDate.set(ntp.year(), ntp.month(), ntp.day());
    // если разрешено, включаем по времени нагрузку ( свет или насос)

    // определяем текущее количество секунд от начала суток
    uint32_t nowSeconds = nowTime.hour * 3600 + nowTime.minute * 60 + nowTime.second;

    // определяем, нужно ли переключить РЕЛЕ 1
    //если нет перехода через полночь
    if (startSeconds1 < stopSeconds1) {
      if (( startSeconds1 <= nowSeconds) && (nowSeconds <= stopSeconds1)) {
        digitalWrite(RELE1, ON);
        setting.rele_1_isOn = 1;
      } else {
        digitalWrite(RELE1, OFF);
        setting.rele_1_isOn = 0;
      }
    }
    //переход через полночь
    else if (startSeconds1 > stopSeconds1) {
      if (( stopSeconds1 <= nowSeconds) && (nowSeconds <= startSeconds1)) {
        digitalWrite(RELE1, OFF);
        setting.rele_1_isOn = 0;
      } else {
        digitalWrite(RELE1, ON);
        setting.rele_1_isOn = 1;
      }
    }

    // определяем, нужно ли переключить РЕЛЕ 2
    //если нет перехода через полночь
    if (startSeconds2 < stopSeconds2) {
      if (( startSeconds2 <= nowSeconds) && (nowSeconds <= stopSeconds2)) {
        digitalWrite(RELE2, ON);
        setting.rele_2_isOn = 1;
      } else {
        digitalWrite(RELE2, OFF);
        setting.rele_2_isOn = 0;
      }
    }
    //переход через полночь
    else if (startSeconds2 > stopSeconds2) {
      if (( stopSeconds2 <= nowSeconds) && (nowSeconds <= startSeconds2)) {
        digitalWrite(RELE2, OFF);
        setting.rele_2_isOn = 0;
      } else {
        digitalWrite(RELE2, ON);
        setting.rele_2_isOn = 1;
      }
    }

    // определяем, нужно ли переключить РЕЛЕ 3
    //если нет перехода через полночь
    if (startSeconds3 < stopSeconds3) {
      if (( startSeconds3 <= nowSeconds) && (nowSeconds <= stopSeconds3)) {
        digitalWrite(RELE3, ON);
        setting.rele_3_isOn = 1;
      } else {
        digitalWrite(RELE3, OFF);
        setting.rele_3_isOn = 0;
      }
    }
    //переход через полночь
    else if (startSeconds3 > stopSeconds3) {
      if (( stopSeconds3 <= nowSeconds) && (nowSeconds <= startSeconds3)) {
        digitalWrite(RELE3, OFF);
        setting.rele_3_isOn = 0;
      } else {
        digitalWrite(RELE3, ON);
        setting.rele_3_isOn = 1;
      }
    }

    // определяем, нужно ли переключить РЕЛЕ 4
    //если нет перехода через полночь
    if (startSeconds4 < stopSeconds4) {
      if (( startSeconds4 <= nowSeconds) && (nowSeconds <= stopSeconds4)) {
        digitalWrite(RELE4, ON);
        setting.rele_4_isOn = 1;
      } else {
        digitalWrite(RELE4, OFF);
        setting.rele_4_isOn = 0;
      }
    }
    //переход через полночь
    else if (startSeconds4 > stopSeconds4) {
      if (( stopSeconds4 <= nowSeconds) && (nowSeconds <= startSeconds4)) {
        digitalWrite(RELE4, OFF);
        setting.rele_4_isOn = 0;
      } else {
        digitalWrite(RELE4, ON);
        setting.rele_4_isOn = 1;
      }
    }
  }//ms2


}//loop()

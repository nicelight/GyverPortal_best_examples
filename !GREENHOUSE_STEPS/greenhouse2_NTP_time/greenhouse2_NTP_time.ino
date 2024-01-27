
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
  // вместо третьего числа "10" будет ваша подсеть ( может быть 1, может 100, какое угодно число)
  // это число подсети нужно указать ниже в настройках, вместо десятки.
  // так же важно указать в настройках все остальные наборы чисел как у вас тут вылезло.
  // не закрывайте это окно, а спуститесь ниже по скетчу, там где пять строк с IPAddress
  // и измените там все как у вас в этом черном окне
*/


// wifi со статическим айпишничком
const char* ssid = "kuso4ek_raya";
const char* password = "1234567812345678";
IPAddress staticIP(192, 168, 10, 201); // важно правильно указать третье число - подсеть, смотри пояснения выше
IPAddress gateway(192, 168, 10, 1);    // и тут изменить тройку на свою подсеть
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 10, 1);       // изменить тройку на свою подсеть
IPAddress dns2(8, 8, 8, 8);

// настройка реле
#define RELE1 12
#define RELE2 13
#define ON 0
#define OFF 1


bool dependByTime = 1; // флаг разрешения включения реле по времени

#include <WiFi.h>       // esp32
#include <GyverNTP.h>
GyverNTP ntp(3);
/* список дополн. серверов
  если "pool.ntp.org" не работает
  "ntp1.stratum2.ru"
  "ntp2.stratum2.ru"
  "ntp.msk-ix.ru"
*/

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS); // для проверки файлов


GPdate nowDate;
/* как пользоваться:
   Установка и считывание даты:
  nowDate.year = 2024;
  nowDate.month = 1;
  nowDate.day = 20;
  nowDate.set(2024, 1, 20);
*/

GPtime nowTime;
GPtime startTime;
GPtime stopTime;
/* как пользоваться:
   Установка и считывание времени:
  nowTime.hour = 10;
  nowTime.minute = 25;
  nowTime.second = 58;
  nowTime.set(12, 30, 0);
*/

int valNum;

uint32_t startSeconds = 0;
uint32_t stopSeconds = 0;

// поддержка wifi связи
void wifiSupport() {
  if (WiFi.status() != WL_CONNECTED) {
    // Подключаемся к Wi-Fi
    Serial.print("try conn to ");
    Serial.print(ssid);
    Serial.print(":");
    WiFi.mode(WIFI_STA);
    if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false) {
      Serial.println("wifi config failed.");
      return;
    }
    WiFi.begin(ssid, password);
    uint8_t trycon = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if (trycon++ < 30) delay(500);
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
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);
  GP.PAGE_TITLE("Greenhouse");
  GP.ONLINE_CHECK();

  //все обновляющиеся параметры на WEB странице надо указать тут
  GP.UPDATE("nowDate,nowTime,startTime,stopTime");
  GP.TITLE("Теплица 2024", "t1");
  GP.HR();
  GP.LABEL("введи настройку: ");
  GP.NUMBER("valNum", "number", valNum); GP.BREAK();
  GP.LABEL("Сегодня: ");
  GP.DATE("nowDate", nowDate);     GP.BREAK();
  GP.LABEL("Время: ");
  GP.TIME("nowTime", nowTime);     GP.BREAK();

  GP.BREAK();
  GP.LABEL("Включим нагрузку в: ");
  GP.TIME("startTime", startTime);     GP.BREAK();
  GP.LABEL("и выключим после: ");
  GP.TIME("stopTime", stopTime);     GP.BREAK();

  GP.BUTTON("btn", "Кнопочка");
  GP.BREAK();
  GP.BREAK();
  GP.BUILD_END();
}


void action() {
  // было обновление
  if (ui.update()) {
    ui.updateDate("nowDate", nowDate);
    ui.updateTime("nowTime", nowTime);
    ui.updateTime("startTime", startTime);
    ui.updateTime("stopTime", stopTime);
  }//ui.update()
  // был клик по компоненту внутри веб странички
  if (ui.click()) {
    if (ui.clickInt("valNum", valNum)) {
      Serial.print("Number: ");
      Serial.println(valNum);
    }
    if (ui.clickDate("nowDate", nowDate)) {
      //      Serial.print("Date: ");
      //      Serial.println(valDate.encode());
    }
    if (ui.clickTime("nowTime", nowTime)) {
      //      Serial.print("Time: ");
      //      Serial.println(valTime.encode());
    }
    // обновление времени запуска и отключения нагрузки
    if (ui.clickTime("startTime", startTime)) {
      startSeconds =  startTime.hour * 60 * 60 +  startTime.minute * 60 +  startTime.second;
    }
    if (ui.clickTime("stopTime", stopTime)) {
      stopSeconds =  stopTime.hour * 60 * 60 +  stopTime.minute * 60 +  stopTime.second;
    }

    if (ui.click("btn")) {
      // здесь обрабатываем нажатие на кнопку
      // Serial.println("Button click");
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
  ntp.setGMT(5); // часовой пояс. Для Москвы: 3
  ntp.setPeriod(600); // обновлять раз в 600 сек

  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  digitalWrite(RELE1, OFF);
  digitalWrite(RELE2, OFF);

}//setup()


void loop() {
  ui.tick();
  ntp.tick();

  // раз в 10 сек проверим стабильность сети
  static uint32_t ms1 = 0;
  if (millis() - ms1 >= 10000) {
    ms1 = millis();
    wifiSupport();
    checkNTPstauts();
  }//ms

  // раз в 1 сек делаем дела
  static uint32_t ms2 = 0;
  if (millis() - ms2 >= 1000) {
    ms2 = millis();
    // отдаем текущую дату и время переменным в веб интерфейс
    nowTime.set(ntp.hour(), ntp.minute(), ntp.second());
    nowDate.set(ntp.year(), ntp.month(), ntp.day());
    // если разрешено, включаем по времени нагрузку ( свет или насос)
    if (dependByTime) {
      uint32_t nowSeconds = nowTime.hour * 3600 + nowTime.minute * 60 + nowTime.second;
      //если нет перехода через полночь
      if (startSeconds < stopSeconds) {
        if (( startSeconds <= nowSeconds) && (nowSeconds <= stopSeconds)) {
          digitalWrite(RELE1, ON);
        } else {
          digitalWrite(RELE1, OFF);
        }
      }
      //переход через полночь
      else if (startSeconds > stopSeconds) {
        if (( stopSeconds <= nowSeconds) && (nowSeconds <= startSeconds)) {
          digitalWrite(RELE1, OFF);
        } else {
          digitalWrite(RELE1, ON);
        }
      }
    }//dependByTime

  }//ms2


}//loop()


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
#define RELE1 14
#define RELE2 13
#define ON 0
#define OFF 1


#include <WiFi.h>       // esp32
#include <GyverNTP.h>
GyverNTP ntp(3);

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS); // для проверки файлов

// для датчика BME280
#include <Wire.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

//настройки, хранятся в памяти EEPROM
struct Settings {
  //  char str[15] = "ello kitty!";
  GPtime startTime;
  GPtime stopTime;
  int16_t minTempr = 0;
  int16_t maxTempr = 0;
  bool dependbyTempr;
  bool rele_1_isOn = 0;
};
Settings setting;

#include <EEPROM.h>
#include <EEManager.h>  // подключаем либу
EEManager  memory(setting); // передаём нашу переменную (фактически её адрес)

bool dependByTime = 1; // флаг разрешения включения реле по времени
GPdate nowDate;
GPtime nowTime;
uint32_t startSeconds = 0;
uint32_t stopSeconds = 0;

float temperature = 0.0;
float humidity = 0.0;




//запускаем датчик температуры
void bme280Init() {
  uint8_t status = bme.begin(0x76);
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2)
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  } else         Serial.println(" BME280 ... OK");
}//bme280Init()


// считываем температуру, влажность
void bme280Read() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");
  //    Serial.print("Pressure = ");
  //    Serial.print(bme.readPressure() / 100.0F);
  //    Serial.println(" hPa");
  //    Serial.print("Approx. Altitude = ");
  //    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  //    Serial.println(" m");
}

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
  GP.BUILD_BEGIN(600);
  GP.THEME(GP_DARK);
  GP.PAGE_TITLE("Greenhouse");
  GP.ONLINE_CHECK();

  //все обновляющиеся параметры на WEB странице надо указать тут
  GP.UPDATE("nowDate,nowTime,startTime,stopTime,tempr,humid,releIndikator");
  GP.TITLE("Теплица 2024", "t1");
  GP.HR();

  GP.LABEL("Сегодня: "); GP.BREAK();
  GP.DATE("nowDate", nowDate);     
  GP.TIME("nowTime", nowTime);     GP.BREAK();
  GP.BREAK();
  GP.LABEL("BME280 параметры ");
  GP.BREAK();
  GP.LABEL("Температура: ");
  GP.LABEL("tempr", "tempr");     
    GP.LABEL("*С");
  GP.BREAK();
  GP.LABEL("Влажность: ");
  GP.LABEL("humid", "humid");     
      GP.LABEL("%");
  GP.BREAK();
  GP.BREAK();
    GP.HR();
  GP.LABEL("Включим нагрузку в: ");
  GP.TIME("startTime", setting.startTime);     GP.BREAK();
  GP.LABEL("и выключим после: ");
  GP.TIME("stopTime", setting.stopTime);     GP.BREAK(); GP.BREAK();
  GP.LABEL("Либо включать по температуре ");
  GP.SWITCH("sw", setting.dependbyTempr);   GP.BREAK();
  GP.LABEL("включать нагрузку при: ");
  GP.NUMBER("minTempr", "number", setting.minTempr);
  GP.LABEL("*С"); GP.BREAK();
  GP.LABEL("отключать при: ");
  GP.NUMBER("maxTempr", "number", setting.maxTempr);
  GP.LABEL("*С"); GP.BREAK();
  GP.BREAK();
   GP.LABEL("Реле 1:");
  GP.LED_RED("releIndikator", setting.rele_1_isOn);
  GP.BREAK();
  GP.BUTTON("btn", "Вкл / Откл");
  GP.BREAK();
  GP.BREAK();
  GP.BUILD_END();
}


void action() {
  // было обновление
  if (ui.update()) {
    ui.updateDate("nowDate", nowDate);
    ui.updateTime("nowTime", nowTime);
    ui.updateTime("startTime", setting.startTime);
    ui.updateTime("stopTime", setting.stopTime);
    ui.updateInt("tempr", temperature);
    ui.updateInt("humid", humidity);
    ui.updateBool("releIndikator", setting.rele_1_isOn);
  }//ui.update()
  // был клик по компоненту внутри веб странички
  if (ui.click()) {
    if (ui.clickDate("nowDate", nowDate)) {
      //      Serial.print("Date: ");
      //      Serial.println(valDate.encode());
    }
    if (ui.clickTime("nowTime", nowTime)) {
      //      Serial.print("Time: ");
      //      Serial.println(valTime.encode());
    }
    // обновление времени запуска и отключения нагрузки
    if (ui.clickTime("startTime", setting.startTime)) {
      startSeconds = setting.startTime.hour * 60 * 60 + setting.startTime.minute * 60 + setting.startTime.second;
      memory.updateNow();
    }
    if (ui.clickTime("stopTime", setting.stopTime)) {
      stopSeconds = setting.stopTime.hour * 60 * 60 + setting.stopTime.minute * 60 + setting.stopTime.second;
      memory.updateNow();
    }
    if (ui.clickBool("sw", setting.dependbyTempr)) {
      memory.updateNow();
    }
    if (ui.clickInt("maxTempr", setting.maxTempr)) {
      memory.updateNow();
    }
    if (ui.clickInt("minTempr", setting.minTempr)) {
      memory.updateNow();
    }
    if (ui.click("btn")) {
      setting.rele_1_isOn = !setting.rele_1_isOn;
      if (setting.rele_1_isOn) digitalWrite(RELE1, ON);
      else digitalWrite(RELE1, OFF);
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
  ntp.setGMT(5); // часовой пояс. Для Москвы: 3
  ntp.setPeriod(600); // обновлять раз в 600 сек
  bme280Init();
  EEPROM.begin(100);  // выделить память (больше или равно размеру структуры данных)
  memory.begin(0, 'e'); // изменить букву в скобках на другую, чтобы восстановить настройки по умолчанию

  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  if (setting.rele_1_isOn)   digitalWrite(RELE1, ON);
  else  digitalWrite(RELE1, OFF);
  digitalWrite(RELE2, OFF);

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

  // раз в 1 сек делаем дела
  static uint32_t ms2 = 0;
  if (millis() - ms2 >= 1000) {
    ms2 = millis();

    bme280Read();

    // отдаем текущую дату и время переменным в веб интерфейс
    nowTime.set(ntp.hour(), ntp.minute(), ntp.second());
    nowDate.set(ntp.year(), ntp.month(), ntp.day());
    // если разрешено, включаем по времени нагрузку ( свет или насос)

    if (setting.dependbyTempr) {
      if ((temperature >= setting.minTempr) && (temperature < setting.maxTempr)) {
        setting.rele_1_isOn = 1;
        digitalWrite(RELE1, ON);
      } else {
        setting.rele_1_isOn = 0;
        digitalWrite(RELE1, OFF);
      }
      // если не от температуры, то включаем по заданному времени
    } else {
      uint32_t nowSeconds = nowTime.hour * 3600 + nowTime.minute * 60 + nowTime.second;
      //если нет перехода через полночь
      if (startSeconds < stopSeconds) {
        if (( startSeconds <= nowSeconds) && (nowSeconds <= stopSeconds)) {
          digitalWrite(RELE1, ON);
          setting.rele_1_isOn = 1;
        } else {
          digitalWrite(RELE1, OFF);
          setting.rele_1_isOn = 0;
        }
      }
      //переход через полночь
      else if (startSeconds > stopSeconds) {
        if (( stopSeconds <= nowSeconds) && (nowSeconds <= startSeconds)) {
          digitalWrite(RELE1, OFF);
          setting.rele_1_isOn = 0;
        } else {
          digitalWrite(RELE1, ON);
          setting.rele_1_isOn = 1;
        }
      }
    }//dependByTime
  }//ms2


}//loop()

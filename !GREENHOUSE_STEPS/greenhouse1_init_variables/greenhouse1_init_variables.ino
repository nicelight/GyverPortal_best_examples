
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
/* как пользоваться:
   Установка и считывание времени:
  nowTime.hour = 10;
  nowTime.minute = 25;
  nowTime.second = 58;
  nowTime.set(12, 30, 0);
*/

int valNum;



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


// конструктор страницы
void build() {
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);
  GP.PAGE_TITLE("Greenhouse");
  GP.ONLINE_CHECK(); 
  
  //все обновляющиеся параметры на WEB странице надо указать тут
  GP.UPDATE("nowDate,nowTime");
  GP.TITLE("Моя Теплица 2024", "t1");
  GP.HR();
  GP.LABEL("введи настройку: ");
  GP.NUMBER("valNum", "number", valNum); GP.BREAK();
  GP.LABEL("Дата: ");
  GP.DATE("nowDate", nowDate);     GP.BREAK();
  GP.LABEL("Время: ");
  GP.TIME("nowTime", nowTime);     GP.BREAK();
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
}//setup()


void loop() {
 // wifiSupport();
  ui.tick();

   // имитируем изменение переменных "откуда то из программы"
    static uint32_t tmr;
    if (millis() - tmr >= 5000) {
    tmr = millis();
    wifiSupport();
//      valNum = random(1000);
//      nowDate.set(random(2000, 2030), random(13), random(13));
//      nowTime.set(random(24), random(60), random(60));
    }
 
}//loop()

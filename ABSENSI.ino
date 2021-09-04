#include <WiFi.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
const char *ssid = "ASUSX555Q";  //Nama Wifi
const char *password = "welcomestark"; // pass wifi
bool Parsing = false;
String command;
String kondisi, nama, code;

#include <Wire.h>
#include <SparkFunMLX90614.h>
IRTherm therm;
//-----------------------------------------//
#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN 27
#define SS_PIN  5
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
//-----------------------------------------//
#include <LiquidCrystal_I2C.h>  //i2C LCD Library
LiquidCrystal_I2C lcd(0x27, 16, 2); //library i2c lcd
//-----------------------------------------//
#include <NewPing.h>
#define TRIGGER_PIN  12
#define ECHO_PIN     14
#define MAX_DISTANCE 20
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
//-----------------------------------------//
#define Buzzer  13
long ultra;
String suhusensor;
//-----------------------------------------//
int period = 200;
unsigned long time_now = 0;

void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  //-----------------------------------------//
  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("ALAT ABSENSI IOT");
  delay(2000);
  lcd.setCursor(0, 1);
  lcd.print("  SISTEM MULAI  ");
  delay(3000);
  lcd.clear();
  //-----------------------------------------//
  therm.begin();
  therm.setUnit(TEMP_C);
  //-----------------------------------------//
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);
  //-----------------------------------------//
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  //  for (byte i = 0; i < 6; i++) {
  //    key.keyByte[i] = 0xFF;
  //  }
  //  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  //  Serial.println();
}

void loop() {
  if (millis() > time_now + period) {
    time_now = millis();
    Running();
  }
}

void Running() {
  lcd.setCursor(0, 0);
  lcd.print("  Silahkan Tap  ");
  lcd.setCursor(0, 1);
  lcd.print("  Untuk Absen ! ");
  while (1) {
    if ( ! mfrc522.PICC_IsNewCardPresent())
      return;
    if ( ! mfrc522.PICC_ReadCardSerial())
      return;
    Serial.print("Card UID :");
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print("PICC type : ");
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println("No valid tag");
      return;
    }
    MFRC522::StatusCode status;
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    cek_data(content.substring(0, 8));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void SensorUltra() {
  ultra = sonar.ping_cm();
}

void Rfid(String nama, String kondisi, String code) {
  if (kondisi == "Benar") {
    Serial.println("This is the right tag");
    Serial.println();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nama);
    lcd.setCursor(0, 1);
    lcd.print("Access di Terima");
    digitalWrite(Buzzer, HIGH);
    delay(100);
    digitalWrite(Buzzer, LOW);
    delay(3000);
    while (1) {
      lcd.setCursor(0, 0);
      lcd.print("  Silahkan Cek  ");
      lcd.setCursor(0, 1);
      lcd.print("  Suhu Anda !   ");
      SensorUltra();
      if (ultra == 6) {
        digitalWrite(Buzzer, HIGH);
        delay(100);
        digitalWrite(Buzzer, LOW);
        delay(100);
        while (1) {
          if (therm.read())
          {
            //            suhusensor = therm.object();
            suhusensor = therm.object() + 2.7;
          }
          double suhu = suhusensor.toDouble();
          if (suhu >= 37.20) {
            while (1) {
              Serial.print("suhu : "); Serial.println(suhu);
              String url = (String)"code=" + code + "&suhu=" + suhu;
              kirim_data(url);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Suhu : ");
              lcd.print(suhu);
              digitalWrite(Buzzer, HIGH);
              delay(1000);
              digitalWrite(Buzzer, LOW);
              delay(100);
              delay(2000);
              lcd.setCursor(0, 1);
              lcd.print("  Terimakasih ! ");
              delay(3000);
              goto Running;
            }
          } else {
            while (1) {
              Serial.print("suhu : "); Serial.println(suhu);
              String url = (String)"code=" + code + "&suhu=" + suhu;
              kirim_data(url);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Suhu : ");
              lcd.print(suhu);
              delay(2000);
              lcd.setCursor(0, 1);
              lcd.print("  Terimakasih ! ");
              delay(3000);
              goto Running;
            }
          }
        }
      } else {
        Serial.print("Ultrasonik : "); Serial.println(ultra);
      }
    }
  } else {
    Serial.println("Wrong tag");
    Serial.println();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access di Tolak!");
    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
    delay(100);
    goto Running;
  }
Running:
  Running();
}

void dump_byte_array(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? "0" : "");
    Serial.print(buffer[i], HEX);
  }
}

void cek_data(String code) {
  String datareq = "http://absensi.promonissanbogor.com/cek_rfid.php";
  datareq += "?code=";
  datareq += code;
  Serial.println(datareq);
  HTTPClient http;
  http.begin(datareq);
  int httpCode = http.GET();
  String payload = http.getString();
  Serial.println(payload);
  JSONVar myObject = JSON.parse(payload);
  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }
  Serial.print("JSON object = ");
  Serial.println(myObject);
  JSONVar keys = myObject.keys();
  kondisi = myObject[keys[2]];
  nama = myObject[keys[1]];
  code = myObject[keys[0]];
  Serial.println(nama);
  Rfid(nama, kondisi, code);
  http.end();
}

void kirim_data(String url) {
  HTTPClient http;
  http.begin("http://absensi.promonissanbogor.com/tambah-proses.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  auto httpCode = http.POST(url);
  String payload = http.getString();
  Serial.println(url);
  Serial.println(payload);
  http.end();
}

#include "WiFi.h"
#include <HTTPClient.h>      //Spread Sheet library HTTP
#include "DFRobot_ESP_PH.h"  //PH Sensor
#include "EEPROM.h"
#include <OneWire.h>
#include <DallasTemperature.h>  //suhu Sensor
#include <FirebaseESP32.h>      //Firebase
#include <Wire.h>
#include <Adafruit_SSD1306.h>  // Memanggil Library OLED SSD1306
#include <string.h>

#define FIREBASE_HOST "https://water-monitoring-system-efc37-default-rtdb.firebaseio.com//"  //untuk database realtime
#define FIREBASE_AUTH "4shKYwmp8Yn1lv4hKbBjBYUJFdVkl8k44rRJwlay"                             // untuk token database secrets
FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;
//===========WiFi credentials==================
//******Wifi Perikanan
// const char* ssid = "ZTE-d2a67a";    // change SSID
// const char* password = "d476ead2";  // change password
//******Wifi Kos
const char* ssid = "Berkah 2";    // change SSID
const char* password = "SELALUBERK4H";  // change password
//  Google script ID and required credentials
String GOOGLE_SCRIPT_ID = "AKfycbwBzegoAYGqQPiq8omNopYfo1aUfu0s6r5MNaKFxK8SAFHx8HUCLHj_7Qq7PMGbsrX3";  // change Gscript ID

// Constants for pH sensor
DFRobot_ESP_PH ph;
#define ESPADC 4096.0                 // The ESP32 Analog Digital Conversion value
#define ESPVOLTAGE 3300               // The ESP32 voltage supply value
#define PH_PIN 35                     // The ESP32 GPIO data pin number
const int oneWireBus = 5;             // Constants for DS18B20 suhu sensor
OneWire oneWire(oneWireBus);          // Setup oneWire instance to communicate with DS18B20
DallasTemperature sensors(&oneWire);  // Setup DallasTemperature to access the sensor

#define SCREEN_WIDTH 128  // Lebar Oled dalam Pixel
#define SCREEN_HEIGHT 64  // Tinggi Oled dalam Pixel
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float minr[26];
float A1[26];
float A2[26];
float voltage, pH, suhu, Z, a1, a2;
String hasil;
String hasilKodular;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);  // Needed to permit storage of calibration value in EEPROM
  ph.begin();
  sensors.begin();  // Initialize the DS18B20 sensor
  Serial.println("DS18B20 Sensor Suhu");
  // connect to WiFi
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Konfigurasi Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  //Display Oled
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // alamat I2C 0x3C untuk 128x28
    Serial.println(F("SSD1306 Gagal"));
    for (;;)
      ;  // mengulang terus, hingga bisa menghubungkan ke I2C Oled
  }
}

// Fungsi keanggotaan untuk input pH air
float fuPSasam(float ph) {
  if (ph >= 5.5) {
    return 0;
  } else if (5 < ph && ph < 5.5) {
    return (5.5 - ph) / (5.5 - 5);
  } else if (ph <= 5) {
    return 1;
  }
  return 0;
}

float fuPasam(float ph) {
  if (ph <= 5 || ph >= 6.5) {
    return 0;
  } else if (5 < ph && ph < 5.5) {
    return (ph - 5) / (5.5 - 5);
  } else if (5.5 <= ph && ph <= 6) {
    return 1;
  } else if (6 < ph && ph < 6.5) {
    return (6.5 - ph) / (6.5 - 6);
  }
  return 0;
}

float fuPnormal(float ph) {
  if (ph <= 6 || ph >= 8) {
    return 0;
  } else if (6 < ph && ph < 7) {
    return (ph - 6) / (7 - 6);
  } else if (ph == 7) {
    return 1;
  } else if (7 < ph && ph < 8) {
    return (8 - ph) / (8 - 7);
  }
  return 0;
}

float fuPbasa(float ph) {
  if (ph <= 7.5 || ph >= 9) {
    return 0;
  } else if (7.5 < ph && ph < 8) {
    return (ph - 7.5) / (8 - 7.5);
  } else if (8 <= ph && ph <= 8.5) {
    return 1;
  } else if (8.5 < ph && ph < 9) {
    return (9 - ph) / (9 - 8.5);
  }
  return 0;
}

float fuPSbasa(float ph) {
  if (ph <= 8.5) {
    return 0;
  } else if (8.5 < ph && ph < 9) {
    return (ph - 8.5) / (9 - 8.5);
  } else if (ph >= 9) {
    return 1;
  }
  return 0;
}

// Fungsi keanggotaan untuk input suhu air
float fuSSdingin(float suhu) {
  if (suhu >= 26) {
    return 0;
  } else if (25 < suhu && suhu < 26) {
    return (26 - suhu) / (26 - 25);
  } else if (suhu <= 25) {
    return 1;
  }
  return 0;
}

float fuSdingin(float suhu) {
  if (suhu <= 25 || suhu >= 28) {
    return 0;
  } else if (25 < suhu && suhu < 26) {
    return (suhu - 25) / (26 - 25);
  } else if (26 <= suhu && suhu <= 27) {
    return 1;
  } else if (27 < suhu && suhu < 28) {
    return (28 - suhu) / (28 - 27);
  }
  return 0;
}

float fuSnormal(float suhu) {
  if (suhu <= 27 || suhu >= 31) {
    return 0;
  } else if (27 < suhu && suhu < 29) {
    return (suhu - 27) / (29 - 27);
  } else if (suhu == 29) {
    return 1;
  } else if (29 < suhu && suhu < 31) {
    return (31 - suhu) / (31 - 29);
  }
  return 0;
}

float fuSpanas(float suhu) {
  if (suhu <= 30 || suhu >= 33) {
    return 0;
  } else if (30 < suhu && suhu < 31) {
    return (suhu - 30) / (31 - 30);
  } else if (31 <= suhu && suhu <= 32) {
    return 1;
  } else if (32 < suhu && suhu < 33) {
    return (33 - suhu) / (33 - 32);
  }
  return 0;
}

float fuSSpanas(float suhu) {
  if (suhu <= 32) {
    return 0;
  } else if (32 < suhu && suhu < 33) {
    return (suhu - 32) / (33 - 32);
  } else if (suhu >= 33) {
    return 1;
  }
  return 0;
}

float RUMUSOutputSangatBaik(float output) {
  a1 = output * (6 - 5) + 5;
  a2 = 7;
  return 0;
}

float RUMUSOutputBaik(float output) {
  a1 = output * (4 - 3) + 3;
  a2 = (-1) * ((output * (6 - 5)) - 6);
  return 0;
}


float RUMUSOutputBuruk(float output) {
  a1 = output * (2 - 1) + 1;
  a2 = (-1) * ((output * (4 - 3)) - 4);
  return 0;
}

float RUMUSOutputSangatBuruk(float output) {
  a1 = 0;
  a2 = (-1) * ((output * (2 - 1)) - 2);
  return 0;
}

//=============================================

float Minn(float a, float b) {
  return (a < b) ? a : b;
}

void rule(float pH, float suhu, float* Rule) {
  // R1 If pH Normal and Suhu Normal then kualitas Sangat_Baik
  Rule[1] = Minn(fuPnormal(pH), fuSnormal(suhu));
  minr[1] = RUMUSOutputSangatBaik(Rule[1]);
  A1[1] = a1;
  A2[1] = a2;

  // R2 If pH Normal and Suhu Panas then kualitas Baik
  Rule[2] = Minn(fuPnormal(pH), fuSpanas(suhu));
  minr[2] = RUMUSOutputBaik(Rule[2]);
  A1[2] = a1;
  A2[2] = a2;

  // R3 If pH Normal and Suhu Dingin then kualitas Baik
  Rule[3] = Minn(fuPnormal(pH), fuSdingin(suhu));
  minr[3] = RUMUSOutputBaik(Rule[3]);
  A1[3] = a1;
  A2[3] = a2;

  // R4 If pH Normal and Suhu Sangat Panas then kualitas Sangat_Buruk
  Rule[4] = Minn(fuPnormal(pH), fuSSpanas(suhu));
  minr[4] = RUMUSOutputSangatBuruk(Rule[4]);
  A1[4] = a1;
  A2[4] = a2;

  // R5 If pH Normal and Suhu Sangat Dingin then kualitas Sangat_Buruk
  Rule[5] = Minn(fuPnormal(pH), fuSSdingin(suhu));
  minr[5] = RUMUSOutputSangatBuruk(Rule[5]);
  A1[5] = a1;
  A2[5] = a2;

  // R6 If pH Asam and Suhu Normal then kualitas Baik
  Rule[6] = Minn(fuPasam(pH), fuSnormal(suhu));
  minr[6] = RUMUSOutputBaik(Rule[6]);
  A1[6] = a1;
  A2[6] = a2;

  // R7 If pH Asam and Suhu Panas then kualitas Buruk
  Rule[7] = Minn(fuPasam(pH), fuSpanas(suhu));
  minr[7] = RUMUSOutputBuruk(Rule[7]);
  A1[7] = a1;
  A2[7] = a2;

  // R8 If pH Asam and Suhu Dingin then kualitas Buruk
  Rule[8] = Minn(fuPasam(pH), fuSdingin(suhu));
  minr[8] = RUMUSOutputBuruk(Rule[8]);
  A1[8] = a1;
  A2[8] = a2;

  // R9 If pH Asam and Suhu Sangat Panas then kualitas Sangat_Buruk
  Rule[9] = Minn(fuPasam(pH), fuSSpanas(suhu));
  minr[9] = RUMUSOutputSangatBuruk(Rule[9]);
  A1[9] = a1;
  A2[9] = a2;

  // R10 If pH Asam and Suhu Sangat Dingin then kualitas Sangat_Buruk
  Rule[10] = Minn(fuPasam(pH), fuSSdingin(suhu));
  minr[10] = RUMUSOutputSangatBuruk(Rule[10]);
  A1[10] = a1;
  A2[10] = a2;

  // R11 If pH Basa and Suhu Normal then kualitas Baik
  Rule[11] = Minn(fuPbasa(pH), fuSnormal(suhu));
  minr[11] = RUMUSOutputBaik(Rule[11]);
  A1[11] = a1;
  A2[11] = a2;

  // R12 If pH Basa and Suhu Panas then kualitas Buruk
  Rule[12] = Minn(fuPbasa(pH), fuSpanas(suhu));
  minr[12] = RUMUSOutputBuruk(Rule[12]);
  A1[12] = a1;
  A2[12] = a2;

  // R13 If pH Basa and Suhu Dingin then kualitas Buruk
  Rule[13] = Minn(fuPbasa(pH), fuSdingin(suhu));
  minr[13] = RUMUSOutputBuruk(Rule[13]);
  A1[13] = a1;
  A2[13] = a2;

  // R14 If pH Basa and Suhu Sangat Panas then kualitas Sangat_Buruk
  Rule[14] = Minn(fuPbasa(pH), fuSSpanas(suhu));
  minr[14] = RUMUSOutputSangatBuruk(Rule[14]);
  A1[14] = a1;
  A2[14] = a2;

  // R15 If pH Basa and Suhu Sangat Dingin then kualitas Sangat_Buruk
  Rule[15] = Minn(fuPbasa(pH), fuSSdingin(suhu));
  minr[15] = RUMUSOutputSangatBuruk(Rule[15]);
  A1[15] = a1;
  A2[15] = a2;

  // R16 If pH Sangat_Asam and Suhu Normal then kualitas Sangat_Buruk
  Rule[16] = Minn(fuPSasam(pH), fuSnormal(suhu));
  minr[16] = RUMUSOutputSangatBuruk(Rule[16]);
  A1[16] = a1;
  A2[16] = a2;

  // R17 If pH Sangat_Asam and Suhu Panas then kualitas Sangat_Buruk
  Rule[17] = Minn(fuPSasam(pH), fuSpanas(suhu));
  minr[17] = RUMUSOutputSangatBuruk(Rule[17]);
  A1[17] = a1;
  A2[17] = a2;

  // R18 If pH Sangat_Asam and Suhu Dingin then kualitas Sangat_Buruk
  Rule[18] = Minn(fuPSasam(pH), fuSdingin(suhu));
  minr[18] = RUMUSOutputSangatBuruk(Rule[18]);
  A1[18] = a1;
  A2[18] = a2;

  // R19 If pH Sangat_Asam and Suhu Sangat Panas then kualitas Sangat_Buruk
  Rule[19] = Minn(fuPSasam(pH), fuSSpanas(suhu));
  minr[19] = RUMUSOutputSangatBuruk(Rule[19]);
  A1[19] = a1;
  A2[19] = a2;

  // R20 If pH Sangat_Asam and Suhu Sangat Dingin then kualitas Sangat_Buruk
  Rule[20] = Minn(fuPSasam(pH), fuSSdingin(suhu));
  minr[20] = RUMUSOutputSangatBuruk(Rule[20]);
  A1[20] = a1;
  A2[20] = a2;

  // R21 If pH Sangat_Basa and Suhu Normal then kualitas Sangat_Buruk
  Rule[21] = Minn(fuPSbasa(pH), fuSnormal(suhu));
  minr[21] = RUMUSOutputSangatBuruk(Rule[21]);
  A1[21] = a1;
  A2[21] = a2;

  // R22 If pH Sangat_Basa and Suhu Panas then kualitas Sangat_Buruk
  Rule[22] = Minn(fuPSbasa(pH), fuSpanas(suhu));
  minr[22] = RUMUSOutputSangatBuruk(Rule[22]);
  A1[22] = a1;
  A2[22] = a2;

  // R23 If pH Sangat_Basa and Suhu Dingin then kualitas Sangat_Buruk
  Rule[23] = Minn(fuPSbasa(pH), fuSdingin(suhu));
  minr[23] = RUMUSOutputSangatBuruk(Rule[23]);
  A1[23] = a1;
  A2[23] = a2;

  // R24 If pH Sangat_Basa and Suhu Sangat Panas then kualitas Sangat_Buruk
  Rule[24] = Minn(fuPSbasa(pH), fuSSpanas(suhu));
  minr[24] = RUMUSOutputSangatBuruk(Rule[24]);
  A1[24] = a1;
  A2[24] = a2;

  // R25 If pH Sangat_Basa and Suhu Sangamt Dingin then kualitas Sangat_Buruk
  Rule[25] = Minn(fuPSbasa(pH), fuSSdingin(suhu));
  minr[25] = RUMUSOutputSangatBuruk(Rule[25]);
  A1[25] = a1;
  A2[25] = a2;
}

float defuzzyfikasi(float* Rule) {
  float MIN = 100;
  float MAX = 0;
  float max_membership = 0;
 

  // Cari nilai maksimum keanggotaan dari semua aturan
  for (int i = 1; i <= 25; i++) {
    if (Rule[i] > max_membership) {
      max_membership = Rule[i];
    }
  }

  for (int i = 1; i <= 25; i++) {
    if (Rule[i] == max_membership) {

      if (A1[i] < MIN) {
        MIN = A1[i];
      }
      if (A2[i] > MAX) {
        MAX = A2[i];
      }
    }
  }

  Z = (((MAX - MIN + 1) * (MIN + MAX)) / 2) / (MAX - MIN + 1);

  return Z;
}

void loop() {
  float Rule[26];
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U)  // Time interval: 1s
  {
    timepoint = millis();
    // Read the suhu from the DS18B20 sensor
    sensors.requestTemperatures();
    suhu = sensors.getTempCByIndex(0);
    // Voltage = rawPinValue / ESPADC * ESPVOLTAGE
    voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE;  // Read the voltage
    Serial.print("Voltage: ");
    Serial.println(voltage, 4);

    // Display suhu
    Serial.print("suhu: ");
    Serial.print(suhu, 1);
    Serial.println(" °C");

    // Convert voltage to pH with suhu compensation
    pH = ph.readPH(voltage, suhu);
    Serial.print("pH: ");
    Serial.println(pH, 4);
  }
  //-----------------------------------
  Serial.print("Nilai pH         : ");
  Serial.println(pH);
  Serial.print("Nilai Suhu       : ");
  Serial.println(suhu);
  Serial.println("===========================================");
  Serial.println("===========   [KEANGGOTAAN SUHU]   ========");

  Serial.print("Sangat dingin    : ");
  Serial.println(fuSSdingin(suhu));
  Serial.print("Dingin           : ");
  Serial.println(fuSdingin(suhu));
  Serial.print("Normal           : ");
  Serial.println(fuSnormal(suhu));
  Serial.print("Panas            : ");
  Serial.println(fuSpanas(suhu));
  Serial.print("Sangat panas     : ");
  Serial.println(fuSSpanas(suhu));
  Serial.println("============   [KEANGGOTAAN PH]   =========");
  Serial.print("Sangat asam      : ");
  Serial.println(fuPSasam(pH));
  Serial.print("Asam             : ");
  Serial.println(fuPasam(pH));
  Serial.print("Normal           : ");
  Serial.println(fuPnormal(pH));
  Serial.print("Basa             : ");
  Serial.println(fuPbasa(pH));
  Serial.print("Sangat basa      : ");
  Serial.println(fuPSbasa(pH));
  Serial.println("===========================================");

  // Terapkan aturan untuk mendapatkan nilai fuzzy
  rule(pH, suhu, Rule);


  Serial.print("Deffuzzyfikasi   : ");
  Serial.println(defuzzyfikasi(Rule));

  Serial.print("Kualitas Air     : ");
  if (defuzzyfikasi(Rule) < 1.50) {
    hasil = "Sangat Buruk";
    hasilKodular = "Sangat-Buruk";
  }

  else if (defuzzyfikasi(Rule) < 3.50 && defuzzyfikasi(Rule) >= 1.50) {
    hasil = "Buruk";
    hasilKodular = "Buruk";
  }

  else if (defuzzyfikasi(Rule) < 5.50 && defuzzyfikasi(Rule) >= 3.50) {
    hasil = "Baik";
    hasilKodular = "Baik";
  }

  else if (defuzzyfikasi(Rule) >= 5.50) {
    hasil = "Sangat Baik";
    hasilKodular = "Sangat-Baik";
  }
  Serial.println(hasil);
  Serial.println();

  delay(5000);
  display.display();
  display.clearDisplay();               //Membersihkan tampilan
  display.setTextSize(1);               //Ukuran tulisan
  display.setTextColor(SSD1306_WHITE);  //Warna Tulisan
  display.setCursor(0, 0);              // Koordinat awal tulisan (x,y) dimulai dari atas-kiri
  display.print("-Monitoring Kwt Air-");
  display.setCursor(0, 20);    // Koordinat awal tulisan (x,y) dimulai dari atas-kiri
  display.print("pH     : ");  //Menampilkan Tulisan RobotikIndonesia
  display.println(pH);
  display.setCursor(0, 32);    // Koordinat awal tulisan (x,y) dimulai dari atas-kiri
  display.print("Suhu   : ");  //Menampilkan nilai variabel dataInt
  display.println(suhu);
  display.setCursor(0, 44);  // Koordinat awal tulisan (x,y) dimulai dari atas-kiri
  display.print("Kwt air: ");
  display.println(hasil);
  display.display();                              //Mulai Menampilkan
                                                  //-------------------- Mengirimkan data suhu dan kelembaban ke Firebase ----------------------
  String nilaipH = String(pH, 2);                 // Membatasi nilai pH menjadi 2 desimal
  String nilaisuhu = String(suhu, 2);
  float nilaidefuzifikasi = defuzzyfikasi(Rule);  //buat variable baru untuk nilai defuzifikasi


  if (Firebase.setString(firebaseData, "/Hasil_Pembacaan/pH", nilaipH)) {
    Serial.println("pH terkirim");
  } else {
    Serial.println("pH tidak terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }

  if (Firebase.setString(firebaseData, "/Hasil_Pembacaan/Suhu", nilaisuhu)) {
    Serial.println("Suhu terkirim");
    Serial.println();
  } else {
    Serial.println("suhu tidak terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }

  if (Firebase.setString(firebaseData, "/Hasil_Pembacaan/Defuzifikasi", hasilKodular)) {
    Serial.println("Hasil Defuzifikasi terkirim");
    Serial.println();
  } else {
    Serial.println("Hasil Defuzifikasi tidak terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }
  //-----------------------------------
  String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "pH=" + String(pH) + "&suhu=" + String(suhu) + "&hasil=" + String(hasil) + "&nilaidefuzifikasi=" + String(nilaidefuzifikasi);
  HTTPClient http;
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  //getting response from google sheet
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Payload: " + payload);
  }
  //---------------------------------------------------------------------
  http.end();
  delay(1000);
}

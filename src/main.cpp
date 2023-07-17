#include <Arduino.h>
#include <bsec.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <MQ135.h>

#define DEVICE "ESP32"

const char* ssid = "I am gonna kill your fucking dog";
const char* password = "morangoscomacucar";

  #define INFLUXDB_URL "https://eu-central-1-1.aws.cloud2.influxdata.com"
  #define INFLUXDB_TOKEN "OvK1qsQXDixTlkudQhyGPr9STmRJH1tNfD-Ma3-T7CqPfEi1i4mqTLZfegJ9Q-ZCp0Ef7t4j_HQUS457ThNt9A=="
  #define INFLUXDB_ORG "a932d6a20e239b6a"
  #define INFLUXDB_BUCKET "humidityandtemperature"
  
  // Time zone info
  #define TZ_INFO "UTC1"
  
  // Declare InfluxDB client instance with preconfigured InfluxCloud certificate
  InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
  
  // Declare Data point
  Point sensor("wifi_status");

/*#include <MQ135.h>
#define ANALOGPIN 34    //  Define Analog PIN on Arduino Board
#define RZERO 206.85    //  Define RZERO Calibration Value
MQ135 gasSensor = MQ135(ANALOGPIN);*/
MQ135 gasSensor = MQ135(34);
int val;
int sensorPin = 34;
int sensorValue = 0;


#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C version

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

int buzzerPin = 16;
int readcounter = 0;

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);

  //RGB LEDS
  pinMode(19, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(17, OUTPUT);


  pinMode(buzzerPin, OUTPUT);
  tone(buzzerPin, 500, 500);delay(500);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.display();
  delay(100);
  display.clearDisplay();
  display.display();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  /////////////////////////////////////////////////////////////////////////////////////////WIFI and DB
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    int i = 0;
    while(WiFi.status() != WL_CONNECTED && i <50){
        Serial.print(".");
        i++;
        delay(100);
    }
    if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected to WiFi");
    Serial.print("Local ESP32 IP: "); Serial.println(WiFi.localIP());
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
    tone(buzzerPin, 500, 250);delay(250);
    tone(buzzerPin, 1000, 250);delay(250);
    }else{
        Serial.println("Can't connect to WIFI");
    }

  
    // Connect to InfluxDB
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: "); Serial.println(client.getServerUrl());
      tone(buzzerPin, 500, 170);delay(170);
      tone(buzzerPin, 1000, 170);delay(170);
      tone(buzzerPin, 1500, 170);delay(170);
    } else {
      Serial.print("InfluxDB "); Serial.println(client.getLastErrorMessage());
    }
    // Add tags to the data sent do db
    sensor.addTag("device", DEVICE);
    sensor.addTag("SSID", "home");
//////////////////////////////////////////////////////////////////////////////////////////////////////////

  /*float rzero = gasSensor.getRZero();
  Serial.print("MQ135 RZERO Calibration Value : "); Serial.println(rzero);*/



  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

}

void loop() {
  
  Serial.println();
  display.setCursor(0,0);
  display.clearDisplay();

  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  Serial.print("BME Temp = "); Serial.print(bme.temperature); Serial.println(" C");
  display.print("BME Temp: "); display.print(bme.temperature); display.println(" C");

  Serial.print("BME Hum = "); Serial.print(bme.humidity); Serial.println(" %");
  display.print("BME Hum: "); display.print(bme.humidity); display.println(" %");

  Serial.print("Pressure = "); Serial.print(bme.pressure / 100.0); Serial.println(" hPa");
  display.print("Pressure: "); display.print(bme.pressure / 100.0); display.println(" hPa");

/////////////////////////////////////////////////////////////////////////////////////////////////////////MQ135 
  /*float ppm = gasSensor.getPPM();
  Serial.print("CO2 = "); Serial.print(ppm);Serial.println(" ppm");
  display.print("CO2: "); display.print(ppm); display.println(" ppm");*/

  val = analogRead(34);
  Serial.print("CO2? = "); Serial.print(val);Serial.println(" ppm");
  display.print("CO2?: "); display.print(val); display.println(" ppm");
////////////////////////////////////////////////////////////////////////////////////////////////////////

  /*Serial.print("Gas = "); Serial.print(bme.gas_resistance / 1000.0); Serial.println(" KOhms");
  display.print("Gas: "); display.print(bme.gas_resistance / 1000.0); display.println(" KOhms");*/

//////////////////////////////////////////////////////////////////////////////////////////////////////// WIFI and DB 

// Clear fields for reusing the point. Tags will remain the same as set above.
    sensor.clearFields();
  
    // Store measured value into point
    //sensor.addField("rssi", WiFi.RSSI());
    sensor.addField("reading", readcounter);
    sensor.addField("temperature", bme.temperature);
    sensor.addField("humidity", bme.humidity);
    sensor.addField("pressure", bme.pressure / 100);
    sensor.addField("co2?", val);
    //sensor.addField("vocgas", bme.gas_resistance/1000);
    
    // Check WiFi and InfluxDB connection
    /*if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wifi not connected");
      display.println("Wifi not connected");
    }else{
      Serial.println("Wifi connected");
      display.println("Wifi connected");
    }*/
  
    /*if (!client.writePoint(sensor)) {
      Serial.print("InfluxDB "); Serial.println(client.getLastErrorMessage());
    }else{
      Serial.println("InfluxDB connected");
      display.println("InfluxDB connected");
    }*/
    //client writepoint writes to database
    //tolineprotocol just text thing?
    if (client.writePoint(sensor) && WiFi.status() == WL_CONNECTED){
          // Print what is being written to InfluxDB
          Serial.print("Writing: "); Serial.println(sensor.toLineProtocol());
    }

    if(bme.humidity >= 52){
      digitalWrite(19, HIGH);
      digitalWrite(18, LOW);
      digitalWrite(17, LOW);
    }else if(bme.humidity < 52 && bme.humidity >= 48){
      digitalWrite(19, LOW);
      digitalWrite(18, HIGH);
      digitalWrite(17, LOW);
    }else{
      digitalWrite(19, LOW);
      digitalWrite(18, LOW);
      digitalWrite(17, HIGH);
    }
    
    display.display();
    readcounter++;
    delay(30000);
}
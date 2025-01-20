#define BLYNK_TEMPLATE_ID "TMPL*******"
#define BLYNK_TEMPLATE_NAME "RemoteAirQualityMonitoring"
#define BLYNK_AUTH_TOKEN ""
#define BLYNK_PRINT Serial
#define BLYNK_DEBUG

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";

// Pin definitions
#define DHTPIN 4      // DHT11 data pin
#define MQ135_PIN 34  // MQ135 analog pin
#define DHTTYPE DHT11

// Constants for alerts
const float TEMP_MAX = 100.0;
const float HUMIDITY_MAX = 85.0;
const int AIR_QUALITY_MAX = 1000;

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Timer for Blynk
BlynkTimer timer;

// Track sensor warmup
unsigned long powerOnTime;
const unsigned long WARMUP_TIME = 24 * 60 * 60 * 1000UL; // 24 hours in milliseconds

// Function to get calibrated PPM value
/*float getCorrectedPPM(float rawADC, float temperature, float humidity) {
  // Convert raw ADC to resistance ratio
  float voltage = rawADC * (3.3 / 4095.0);  // ESP32 ADC is 12-bit (0-4095)
  float rs = ((3.3 * RLOAD) / voltage) - RLOAD;
  float ratio = rs / RZERO;
  
  // Get raw ppm
  float ppm = PARA * pow((ratio), -PARB);
  
  // Temperature and humidity corrections
  if (!isnan(temperature) && !isnan(humidity)) {
    ppm *= (1.0 + 0.02 * (temperature - 20.0));  // 2% change per degree from 20°C
    ppm *= (1.0 + 0.02 * (humidity - 65.0));     // 2% change per % from 65% RH
  }
  
  return ppm;
}
// MQ135 calibration parameters
//#define RLOAD 10.0    // Load resistance in kΩ
//#define RZERO 76.63   // Calibration resistance at atmospheric CO2 level
//#define PARA 116.6020682    // Linearizing parameter
//#define PARB 2.769034857    // Linearizing parameter
*/


// Function to read and send sensor data
void sendSensorData() {
  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  // Read MQ135 sensor
  int airQuality = analogRead(MQ135_PIN);
  // Check if sensor is still in warmup period
  if (millis() - powerOnTime < WARMUP_TIME) {
    Serial.println("MQ135 still in warmup period. Readings may not be accurate.");
    Serial.print("Hours remaining: ");
    Serial.println((WARMUP_TIME - (millis() - powerOnTime)) / 3600000.0);
  }
  delay(2000);  // DHT11 needs 2 seconds between readings
  // Check if any reads failed
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.println("Attempting sensor reset...");
    dht.begin();
    return;
  }
  // Send data to Blynk
  Blynk.virtualWrite(V0, temperature);  // Temperature on Virtual Pin 0
  Blynk.virtualWrite(V1, humidity);     // Humidity on Virtual Pin 1
  Blynk.virtualWrite(V2, airQuality);   // Air Quality on Virtual Pin 2
  // Print values to Serial Monitor
  Serial.println("Sensor Readings:");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Air Quality (CO2 PPM): ");
  Serial.println(airQuality);
  // Check for alerts
  if (temperature > TEMP_MAX) {
    Blynk.logEvent("temp_alert", String("Temperature is too high: ") + temperature + "°C");
  }
  if (humidity > HUMIDITY_MAX) {
    Blynk.logEvent("humidity_alert", String("Humidity is too high: ") + humidity + "%");
  }
  if (airQuality > AIR_QUALITY_MAX) {
    Blynk.logEvent("air_quality_alert", String("Poor air quality detected: ") + airQuality +"PPM");
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  pinMode(DHTPIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  
  // Initialize DHT sensor
  dht.begin();

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // Setup a timer to trigger every 30 seconds
  timer.setInterval(60000L, sendSensorData);
  powerOnTime = millis();  // Start tracking warmup time

  Serial.println("System initialized!");
}

void loop() {
  Blynk.run();
  timer.run();
}

#include <LiquidCrystal.h>
#include <DHT.h>

// Pin Configuration
#define DHTPIN 8
#define DHTTYPE DHT11
#define ALERT_LED_PIN 13

// High temperature threshold for alert (in Celsius)
const float TEMP_THRESHOLD = 28.0;

// Initialize hardware objects
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
DHT dht(DHTPIN, DHTTYPE);

// Custom degree symbol bitmap (5x8)
byte degreeSymbol[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// Global variables for min/max tracking
float minTemp = 100.0, maxTemp = -100.0;
float minHum = 100.0, maxHum = 0.0;

// Timer variable for non-blocking screen toggling
unsigned long lastDisplayChange = 0;
bool showMinMaxPage = false;

void setup() {
  Serial.begin(9600);
  pinMode(ALERT_LED_PIN, OUTPUT);
  
  dht.begin();
  lcd.begin(16, 2);
  
  // Register custom degree symbol in slot 0
  lcd.createChar(0, degreeSymbol);
  
  // Startup screen
  lcd.setCursor(0, 0);
  lcd.print("DHT11 Station");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read humidity and temperatures
  float h = dht.readHumidity();
  float tC = dht.readTemperature();
  float tF = dht.readTemperature(true);

  // Check for sensor read failure
  if (isnan(h) || isnan(tC) || isnan(tF)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error   ");
    delay(2000);
    return;
  }

  // 1. Calculate Heat Index
  float heatIndexC = dht.computeHeatIndex(tC, h, false);

  // 2. Update Min/Max tracking
  if (tC < minTemp) minTemp = tC;
  if (tC > maxTemp) maxTemp = tC;
  if (h < minHum) minHum = h;
  if (h > maxHum) maxHum = h;

  // 3. Threshold Alert Logic
  if (tC >= TEMP_THRESHOLD) {
    digitalWrite(ALERT_LED_PIN, HIGH);
  } else {
    digitalWrite(ALERT_LED_PIN, LOW);
  }

  // 4. Log everything to Serial Monitor
  Serial.print(F("Temp: ")); Serial.print(tC); Serial.print(F("°C / "));
  Serial.print(tF); Serial.print(F("°F | Hum: ")); Serial.print(h);
  Serial.print(F("% | Heat Index: ")); Serial.print(heatIndexC); Serial.println(F("°C"));

  // 5. Alternate LCD view every 3 seconds between Live Data and Min/Max Stats
  if (millis() - lastDisplayChange > 3000) {
    lastDisplayChange = millis();
    showMinMaxPage = !showMinMaxPage;
    lcd.clear();
  }

  if (!showMinMaxPage) {
    // Page 1: Current Live Readings
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tC, 1);
    lcd.write(byte(0)); // Custom degree symbol
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hum:  ");
    lcd.print(h, 1);
    lcd.print("%");
  } else {
    // Page 2: Min/Max Summary
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print((int)minTemp);
    lcd.print("-");
    lcd.print((int)maxTemp);
    lcd.write(byte(0));
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print((int)minHum);
    lcd.print("-");
    lcd.print((int)maxHum);
    lcd.print("%");
  }

  delay(2000); // Sensor update rate interval
}
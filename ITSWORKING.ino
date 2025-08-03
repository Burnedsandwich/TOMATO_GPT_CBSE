
#include <SoftwareSerial.h>
#include "DHT.h"
#include <SimpleDHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Pin Definitions
#define SOIL_SENSOR_PIN A1     // Pin for soil moisture sensor
#define SENSOR_PIN 7           // Pin for fire sensor
#define BUZZER_PIN 3           // Pin for buzzer
#define RELAY_PIN 8            // Pin for relay to control the sprinkler
#define LED_FIRE_PIN 6         // Pin for fire status LED
#define RAIN_SENSOR_PIN A3     // Pin for rain sensor
#define SERVO_PIN 9            // Pin for servo motor
#define LDR_PIN A2             // Pin for LDR module (can be another pin if A0 is used by soil sensor)

// Sprinkler timing parameters
#define SPRINKLER_START_DELAY 5000  // 5 seconds delay before starting sprinkler
#define SPRINKLER_ON_TIME 3000      // 3 seconds sprinkler on time
#define SOIL_MOISTURE_THRESHOLD 800
#define TEMPERATURE_THRESHOLD 35    // Temperature threshold for relay activation
#define BUZZER_INTENSITY 50

// DHT11 Sensor Pin and LCD setup
int pinDHT11 = 2;
SimpleDHT11 dht11(pinDHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servo; // Create servo object to control a servo

// Variables for sensors
unsigned long previousTime = 0; // Timer for sprinkler activation
bool sprinklerActive = false;   // Flag to track if the sprinkler is active
int angle = 0;                  // Current angle of servo motor
int prev_rain_state = HIGH;     // Initialize to HIGH for stable detection
int rain_state;                 // Current state of rain sensor
unsigned long lastRainChange = 0; // Last time the rain state changed
const unsigned long rainDebounceDelay = 200; // Debounce delay for rain sensor
bool isRaining = false;         // Tracks current rain state

SoftwareSerial B(10, 11); // 10-RX, 11-TX (Bluetooth communication)
#define DHTPIN 2       // Pin connected to DHT11 sensor
#define DHTTYPE DHT11  // DHT11 sensor type

DHT dht(DHTPIN, DHTTYPE);  // Initialize DHT sensor

void setup() {
    Serial.begin(9600);   // Begin Serial communication for debugging
    B.begin(9600);        // Begin Bluetooth communication

    dht.begin();          // Initialize the DHT11 sensor
    lcd.begin();          // LCD initialization
    lcd.backlight();
    servo.attach(SERVO_PIN);
    servo.write(angle); // Initialize servo position

    // Pin setup
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    pinMode(SENSOR_PIN, INPUT);  // Fire sensor input
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_FIRE_PIN, OUTPUT);
    pinMode(RAIN_SENSOR_PIN, INPUT); // Rain sensor input
    pinMode(LDR_PIN, INPUT);        // LDR pin as input

    // Initial rain state setup
    isRaining = digitalRead(RAIN_SENSOR_PIN) == LOW;
}

void loop() {
    int moistureLevel = analogRead(SOIL_SENSOR_PIN);
    int ldrValue = analogRead(LDR_PIN); // Read the value from the LDR sensor

    // Sensor handling functions
    handleFireDetection();
    handleSoilMoisture(moistureLevel);
    readDHT11();
    handleRainSensor();
    handleLDR(ldrValue); // Handle LDR logic

    // Send data over Bluetooth every 5 seconds
    if (millis() - previousTime >= 5000) {
        sendSensorData(); // Send the sensor data
        previousTime = millis(); // Reset the timer
    }

    delay(400); // Delay for stability between sensor readings
}

void sendSensorData() {
    // Read the sensors again for sending updated data
    float humidity = dht.readHumidity();
    float temperatureC = dht.readTemperature();
    float temperatureF = dht.readTemperature(true); // Fahrenheit

    if (isnan(humidity) || isnan(temperatureC)) {
        B.println("Failed to read from DHT sensor!");
        return;
    }

    // Read fire sensor
    int fireStatus = digitalRead(SENSOR_PIN);
    String fireStatusMessage = (fireStatus == LOW) ? "Fire detected" : "No fire";

    // Send the data over Bluetooth
    B.print("Humidity: ");
    B.print(humidity);
    B.print("%, ");
    B.print("Temp: ");
    B.print(temperatureC);
    B.print("C, ");
    B.print("Temp: ");
    B.print(temperatureF);
    B.print("F;");

    B.print("Soil Moisture: ");
    B.print(analogRead(SOIL_SENSOR_PIN));
    B.print(";");

    B.print("Rain State: ");
    B.print(isRaining ? "Rain detected" : "No rain");
    B.print(";");

    B.print("Fire Status: ");
    B.println(fireStatusMessage);

    // Debugging information (optional, can be removed if not needed)
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%, ");
    Serial.print("Temp: ");
    Serial.print(temperatureC);
    Serial.print("C, ");
    Serial.print("Temp: ");
    Serial.print(temperatureF);
    Serial.println("F");

    Serial.print("Fire Status: ");
    Serial.println(fireStatusMessage);
}

void handleFireDetection() {
    static unsigned long previousTime = millis();
    int sensorValue = digitalRead(SENSOR_PIN);

    if (sensorValue == LOW) { // Fire detected
        analogWrite(BUZZER_PIN, BUZZER_INTENSITY);
        if (millis() - previousTime > SPRINKLER_START_DELAY) {
            digitalWrite(RELAY_PIN, LOW); // Activate sprinkler
            delay(SPRINKLER_ON_TIME); // Keep sprinkler on for a certain time
        }
    } else {
        analogWrite(BUZZER_PIN, 0); // Turn off buzzer
        digitalWrite(RELAY_PIN, HIGH); // Turn off sprinkler
        previousTime = millis(); // Reset timer
    }
}

void handleSoilMoisture(int moistureLevel) {
    if (moistureLevel < SOIL_MOISTURE_THRESHOLD) {
        digitalWrite(RELAY_PIN, HIGH);  // Turn on sprinkler
        Serial.println("Relay ON: Soil is dry.");
    } else {
        digitalWrite(RELAY_PIN, LOW);  // Turn off sprinkler
        if (moistureLevel >= SOIL_MOISTURE_THRESHOLD) {
            Serial.println("Relay OFF: Soil is moist.");
        } else if (isRaining) {
            Serial.println("Relay OFF: It's raining, no need to water.");
        }
    }
}

void handleRainSensor() {
    int currentRainState = digitalRead(RAIN_SENSOR_PIN);

    // Debounce logic
    if (currentRainState != prev_rain_state && (millis() - lastRainChange) > rainDebounceDelay) {
        lastRainChange = millis();
        if (currentRainState == LOW) { // Rain detected
            Serial.println("Rain detected!");
            isRaining = true;
            servo.write(90); // Move servo to position 90 degrees
        } else { // Rain stopped
            Serial.println("Rain stopped!");
            isRaining = false;
            servo.write(0); // Move servo back to original position, regardless of soil moisture level
        }
        prev_rain_state = currentRainState; // Update previous state
    }
}

void handleLDR(int ldrValue) {
    // Control the LED based on LDR value (darkness = turn on LED)
    if (ldrValue < 500) { // If it's dark, turn the LED on
        digitalWrite(LED_FIRE_PIN, LOW); // Turn on LED
    } else { // If it's light, turn the LED off
        digitalWrite(LED_FIRE_PIN, HIGH); // Turn off LED
    }
}

void readDHT11() {
    byte temperature = 0;
    byte humidity = 0;
    int err = dht11.read(&temperature, &humidity, NULL);

    if (err != SimpleDHTErrSuccess) {
        Serial.print("Read DHT11 failed, err=");
        Serial.println(err);
        displayError();
    } else {
        displayData(temperature, humidity);

        if (temperature > TEMPERATURE_THRESHOLD) {
            Serial.println("Temperature above 35°C, activating relay.");
            digitalWrite(RELAY_PIN, LOW); // Turn on sprinkler
        } else {
            digitalWrite(RELAY_PIN, HIGH); // Turn off sprinkler if temperature is below threshold
        }
    }
}

void displayData(byte temperature, byte humidity) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print((int)temperature);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print((int)humidity);
    lcd.print(" %");

    Serial.print("Temperature: ");
    Serial.println((int)temperature);
    Serial.print("Humidity: ");
    Serial.println((int)humidity);
}

void displayError() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Save");
    lcd.setCursor(0, 1);
    lcd.print("Farmers");
    delay(2000); // Display error for 2 seconds
}

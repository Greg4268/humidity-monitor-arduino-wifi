#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GenericDevice.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>

WiFiSSLClient client;

char ssid[] = "****";
char pass[] = "********";

const char* server = "web-production-6160.up.railway.app"; 
const int port = 443;
const char* path = "/update-alert";

// LCD Display pins 
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// LED pins 
const int RED_LED = 10, YELLOW_LED = 9, GREEN_LED = 8;

enum humidityRanges {
  HIGH = 1,
  CLOSE_HIGH = 2, 
  CLOSE_LOW = 3, 
  LOW = 4, 
  GOOD = 5 
};

struct sensorState {
  float humidity,
  float temperature, 
  humidityRanges condition, 
  time_t timestamp
};

// moist terrarium humidity constants 
enum terrarium {
  TERRARIUM_LOW = 60, 
  TERRARIUM_CLOSE_LOW = 65, 
  TERRARIUM_CLOSE_HIGH = 88, 
  TERRARIUM_HIGH = 93
};

// humidity constants for indoors - not terrarium 
enum indoor {
  INDOOR_LOW = 30, 
  INDOOR_CLOSE_LOW = 35, 
  INDOOR_CLOSE_HIGH = 55, 
  INDOOR_HIGH = 60
};

const CLEAN_SENSOR_THRESHOLD = 70; 

void connectToWiFi();
void sendAlertToServer(sensorState state);
char isHumidityGoodBadOrBetweenTerrarium(int currHumidity);
char isHumidityGoodBadOrBetweenIndoor(int currHumidity);
void lowHumidityWarningLCD();
void closeHumidityWarningLCD();
void highHumidityWarningLCD();
void triggerLEDAndBuzzer(char condition);

bool measureIndoor = true;
bool measureTerrarium = !measureIndoor;

//buzzer 
const int buzzer = 6;

// humidity and temperature sensor 
Adafruit_Si7021 sensor = Adafruit_Si7021();
bool enableHeater = false;

sensorState state; 

//  for status tracking and sending
bool firstRun = true;  // Flag to track first run
unsigned long lastStatusSentTime = 0;
const unsigned long statusSendInterval = 300000;  // Send status update every 5 minutes (300,000 ms) 

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  // begin serial 
  Serial.begin(115200);
  while(!Serial) delay(10);
  delay(500);
  Serial.println("Found Si7021");

  // connect to WiFi
  bool connected = connectToWiFi(1);
  if(!connected){
    print("Error connecting to wifi on initial setup. Exiting.")
    return
  };

  // start lcd 
  Serial.println("Starting LCD connection");
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Humidity Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  previousStatus = sensor.readHumidity();
}

void loop() {
  // Save the previous status before updating
  previousStatus = state.condition;
  getSensorReadings();

  Serial.print("Humidity: ");
  Serial.println(state.humidity);

  triggerLEDAndBuzzer(state);
  triggerLCD(state);

  // Send alert only if status has changed or it's the first run or periodic update time
  unsigned long currentTime = millis();
  if (firstRun || state.condition != previousStatus || 
      (currentTime - lastStatusSentTime >= statusSendInterval)) {
    
    sendAlertToServer(state.condition);
    lastStatusSentTime = currentTime;
    
    if (firstRun) {
      firstRun = false;
      Serial.println("Initial status sent");
    } else if (state.condition != previousStatus) {
      Serial.println("Status changed - alert sent");
    } else {
      Serial.println("Periodic status update sent");
    }
  }

  cleanSensor(state);
}


char isHumidityGoodBadOrBetweenTerrarium(int humidity){
  if(humidity >= TERRARIUM_HIGH){
    return HIGH;
  }
  else if (humidity <= TERRARIUM_LOW) {
    return LOW; 
  }
  if(humidity >= TERRARIUM_CLOSE_HIGH){
    return CLOSE_HIGH;
  }
  else if (humidity <= TERRARIUM_CLOSE_LOW) {
    return CLOSE_LOW;
  }
  return GOOD;
} 

char isHumidityGoodBadOrBetweenIndoor(int humidity){
  if(humidity >= INDOOR_HIGH || humidity <= INDOOR_LOW) {
    return HIGH;
  }
  else if(humidity <= INDOOR_LOW) {
    return LOW 
  }
  if(humidity >= INDOOR_CLOSE_HIGH) {
    return CLOSE_HIGH;
  }
  else if(humidity <= INDOOR_CLOSE_LOW) {
    return CLOSE_LOW;
  }
  return GOOD;
} 

void triggerLCD(sensorState state) {
  switch(state.condition) {
    case HIGH: 
      lcd.clear();
      for(byte i = 0; i < 3; i++){
        lcd.print("WARNING");
        delay(200);
        lcd.clear();
        delay(50);  
      }
      lcd.print("Humidity HIGH!");
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(sensor.readHumidity(), 1);
      lcd.print("%");
      break;
    case CLOSE_HIGH:
      lcd.clear();
      lcd.print("Out of ideal");
      delay(1000);
      lcd.clear();
      lcd.print("humidity range");
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(sensor.readHumidity(), 1);
      lcd.print("%");
      break;
    case GOOD:
      lcd.clear();
      lcd.print("Humidity: GOOD");
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(currHumidity);
      lcd.print("%");
      break;
    case CLOSE_LOW:
      lcd.clear();
      lcd.print("Out of ideal");
      delay(1000);
      lcd.clear();
      lcd.print("humidity range");
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(sensor.readHumidity(), 1);
      lcd.print("%");
      break;
    case LOW:
      lcd.clear();
      for(byte i = 0; i < 3; i++){
        lcd.print("WARNING");
        delay(200);
        lcd.clear();
        delay(50);
      }
      lcd.print("Humidity LOW!");
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(sensor.readHumidity(), 1);
      lcd.print("%");
      break;
    default:
      println("There was an unknown error with the input.")
      break;
  }
}

void triggerLEDAndBuzzer(sensorState state) {
  // Turn all LEDs off first
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  noTone(buzzer);
  
  if(state.condition == GOOD){
    digitalWrite(GREEN_LED, HIGH);
    delay(200);
  }
  else if(state.condition == HIGH || state.condition == LOW){
    digitalWrite(RED_LED, HIGH);
    tone(buzzer, 500);
    delay(200);
  }
  else if (state.condition == CLOSE_HIGH || state.condition == CLOSE_LOW) {
    digitalWrite(YELLOW_LED, HIGH);
    tone(buzzer, 800);
    delay(200);
  }
}

void sendAlertToServer(sensorState state) {
  status = state.condition;
  String statusString;

  // this could be done better 
  // statusString are weird 
  switch(status) {
    case GOOD:
      statusString = "good";
      break;
    case CLOSE_HIGH:
      statusString = "warning(approaching bad)";
      break;
    case CLOSE_LOW:
      statusString = "warning(approaching bad)";
      break;
    case LOW:
      statusString = "warning(bad)";
      break;
    case HIGH: 
      statusString = "warning(bad)";
      break;
  }

  Serial.print("Sending alert status: ");
  Serial.println(statusString);
  
  // Create JSON document
  StaticJsonDocument<200> doc;
  doc["status"] = statusString;
  
  String jsonString;
  serializeJson(doc, jsonString);

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected; Attempting to reconnect...");
    connectToWiFi(1); // retries 3 times, 45 seconds total checking at 500ms increments 
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect. Cannot send alert ;(");
      return;
    }
  }
  
  // Connect to server with timeout
  Serial.print("Connecting to server...");
  unsigned long connectionStartTime = millis();
  boolean connected = false;
  
  // Try to connect with timeout
  while (millis() - connectionStartTime < 5000) { // 5 second timeout
    if (client.connect(server, port)) {
      connected = true;
      break;
    }
    delay(100);
    Serial.print(".");
  }
  
  if (connected) {
    Serial.println("connected to server!");
    
    // Send the HTTP request
    client.println("POST " + String(path) + " HTTP/1.1");
    client.println("Host: " + String(server));
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(jsonString.length()));
    client.println("Connection: close");
    client.println();
    client.println(jsonString);
    
    // Wait for response with timeout
    Serial.println("Waiting for response...");
    unsigned long responseStartTime = millis();
    boolean responseReceived = false;
    
    while (client.connected() && millis() - responseStartTime < 10000) {
      if (client.available()) {
        responseReceived = true;
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    
    if (!responseReceived) {
      Serial.println("No response received from server (timeout)");
    }
    
    client.stop();
    Serial.println("Connection closed");
  } else {
    Serial.println("connection failed!");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
    Serial.print("Signal strength (RSSI): ");
    Serial.println(WiFi.RSSI());
  }
}

// flow
// attempt to connect -> check status 30 times with 500ms delays then confirm connection -> else: print status, recall function up to 3 times or 45 seconds total 
bool connectToWiFi(int retries = 1) {
  if(retries > 3) return false; // avoid infinite loop 
  // Initialize connection counter
  int connectionAttempts = 0;
  const int maxAttempts = 30;  // Maximum number of attempts (15 seconds) => (30 attemps * 500 millisecond delay)
  
  // Start WiFi connection
  WiFi.begin(ssid, pass);
  
  // Wait for WiFi to connect 
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }
  
  // Check if connected successfully
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status())
    Serial.println("Retrying connection...");
    connectToWiFi(connectCalls + 1);  
  }
  return true;
}

void getSensorReadings() {
  state.humidity = sensor.readHumidity();
  state.temperature = sensor.readTemperature();

  if(measureIndoor){
    state.condition = isHumidityGoodBadOrBetweenIndoor(state.humidity);
  } else {
    state.condition = isHumidityGoodBadOrBetweenTerrarium(state.humidity);
  }

  // get a timestamp of these readings 
  state.timestamp = 1; // placeholder 
}

void cleanSensor(sensorState state) {
  // Toggle heater to clean sensor (use sparingly when high humidity is achieved)
  if (state.humidity > CLEAN_SENSOR_THRESHOLD) {
    enableHeater = !enableHeater;
    sensor.heater(enableHeater);
    Serial.print("Heater Enabled State: ");
    if (sensor.isHeaterEnabled()) {
      Serial.println("ENABLED");
      delay(3000); // run for only 3 seconds 
      enableHeater = !enableHeater;
    } else {
      Serial.println("DISABLED");
    }
  }
}
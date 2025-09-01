#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Keep this

#define WATER_SENSOR_PIN A0
#define RELAY_PIN 14  // D5 (GPIO14)
#define SOIL_PIN A0  // Analog pin for soil moisture
#define DHTPIN 4         // D2 = GPIO4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);



const char* ssid = "*****";
const char* password = "****";

const char* mqtt_server = "broker.hivemq.com";
const char* MQTT_username = NULL;
const char* MQTT_password = NULL;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long previousMillis = 0;

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
   if (client.connect("NodeMCUClient", MQTT_username, MQTT_password)) {
      Serial.println("connected");
      client.subscribe("iotfrontier/pump");  // ✅ Subscribe to pump topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" → retrying in 5 sec");
      delay(5000);
    }
  }
}


void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with pump OFF
  dht.begin();
  delay(1000); // ✅ let DHT22 warm up
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  // Add this to handle incoming commands
  Wire.begin(0, 5); // GPIO0 = D3 (SDA), GPIO5 = D1 (SCL)
lcd.begin(16, 2);  // 16 columns, 2 rows
lcd.backlight();
lcd.backlight();  // Turn on LCD backlight
lcd.setCursor(0, 0);
lcd.print("Smart Farm Ready");
delay(2000);
lcd.clear();

}
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == "iotfrontier/pump") {
    if (message == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Pump ON");
    } else if (message == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Pump OFF");
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > 10000) {
    previousMillis = currentMillis;

    float humidity = dht.readHumidity();
    float temperatureC = dht.readTemperature();
    
    int soilRaw = analogRead(SOIL_PIN);
    int soilPercent = map(soilRaw, 1023, 0, 0, 100);  // dry to wet

    int waterRaw = analogRead(WATER_SENSOR_PIN);
    int waterPercent = map(waterRaw, 180, 920, 0, 100); // Calibrated

    // 🧠 OPTIONAL: Reverse the map if your sensor gives 1023 = empty
    // int waterPercent = map(waterRaw, 1023, 0, 0, 100);

    if (!isnan(humidity) && !isnan(temperatureC)) {
      client.publish("iotfrontier/temperature", String(temperatureC).c_str());
      client.publish("iotfrontier/humidity", String(humidity).c_str());
    }

    client.publish("iotfrontier/soil", String(soilPercent).c_str());
    client.publish("iotfrontier/waterlevel", String(waterPercent).c_str());

    // Debug
    Serial.print("Water Raw: ");
    Serial.print(waterRaw);
    Serial.print(" → ");
    Serial.print("Water Level: ");
    Serial.print(waterPercent);
    Serial.println(" %");

    lcd.clear();
lcd.setCursor(0, 0);
lcd.print("T:");
lcd.print(temperatureC);
lcd.print("C H:");
lcd.print(humidity);
lcd.print("%");

lcd.setCursor(0, 1);
lcd.print("S:");
lcd.print(soilPercent);
lcd.print("% W:");
lcd.print(waterPercent);
lcd.print("%");

  }
}
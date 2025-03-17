#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// WiFi Credentials
const char* ssid = "KEVIN";  
const char* password = "12345678";  

// Telegram Bot Credentials
const char* botToken = "7762005747:AAG5Z5HASp739noe7Jeibxe-dJx57ZzgHyI";  
const char* chatId = "1478399823";  

WiFiClientSecure client;
const char* telegramHost = "api.telegram.org";

// Hardware Pins
#define TRIG_PIN D6
#define ECHO_PIN D7
#define BUZZER_PIN D3
#define GPS_TX D4
#define GPS_RX D5

SoftwareSerial gpsSerial(GPS_TX, GPS_RX); // GPS Communication

// Timing Variables
long lastCheck = 0;  
int checkInterval = 5000;  
long lastGPSUpdate = 0;
int gpsUpdateInterval = 10000;  // 5 minutes (300000ms)

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    client.setInsecure();  
}

void loop() {
    if (Serial.available()) {
        String message = Serial.readStringUntil('\n');
        message.trim();
        if (message.length() > 0) {
            sendMessageToTelegram(message);
        }
    }

    
    // Check for obstacle detection
    if (detectObstacle()) {
        Serial.println("Obstacle detected within 20cm!");
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Send GPS location every 5 minutes
    if (millis() - lastGPSUpdate > gpsUpdateInterval) {
        lastGPSUpdate = millis();
        sendGPSLocationToTelegram();
    }
}

// Function to Send Message to Telegram
void sendMessageToTelegram(String text) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }

    client.connect(telegramHost, 443);

    String url = "/bot" + String(botToken) + "/sendMessage?chat_id=" + String(chatId) + "&text=" + text;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + telegramHost + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Message sent to Telegram: " + text);
}


// Function to Detect Obstacles
bool detectObstacle() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    int distance = duration * 0.034 / 2; // Convert time to distance

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    return (distance > 0 && distance <= 20); // Obstacle detected within 20 cm
}

// Function to Get GPS Location and Send to Telegram
void sendGPSLocationToTelegram() {
    String gpsData = getGPSData();
    if (gpsData != "") {
        sendMessageToTelegram("📍 GPS Location: " + gpsData);
    } else {
              // sendMessageToTelegram("📍 Person Lost");
        Serial.println("GPS data not available.");
    }
}


String getGPSData() {
    String gpsString = "";
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        gpsString += c;
    }
        Serial.println(gpsString);

  
    if (gpsString.indexOf("$GPGGA") != -1) {
        int start = gpsString.indexOf("$GPGGA");
        int end = gpsString.indexOf("\n", start);
        String gpggaData = gpsString.substring(start, end);

        Serial.println("Raw GPS Data: " + gpggaData);

        String lat = extractGPSValue(gpggaData, 2);  // Latitude
        String lon = extractGPSValue(gpggaData, 4);  // Longitude

        return lat + "," + lon;
    }
    return "";
}

String extractGPSValue(String data, int index) {
    int commaCount = 0;
    int startIndex = 0;
    int endIndex = 0;

    for (int i = 0; i < data.length(); i++) {
        if (data[i] == ',') {
            commaCount++;
            if (commaCount == index) {
                startIndex = i + 1;
            } else if (commaCount == index + 1) {
                endIndex = i;
                break;
            }
        }
    }
    return data.substring(startIndex, endIndex);
}
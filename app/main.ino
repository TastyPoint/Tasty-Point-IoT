#include <WiFi.h> //Library to connect ESP32 to WiFi network
#include <WiFiClient.h>
#include <time.h>
#include "DHTesp.h"
#include <FirebaseESP32.h> //Library to ESP32 communicate with Firebase
#include <cstring>

#include <string>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
// Defining the WiFi channel speeds up the connection
#define WIFI_CHANNEL 6

// Provide the token generation process info.
#include "addons/TokenHelper.h"

// Insert Firebase project API Key
// #define API_KEY "AIzaSyDjk-mtrSASnfCGy_SxiCyD0TK2QCNxOBU"
#define API_KEY "AIzaSyDga5aiwXmw_YZOGAKOOPUifrKd85GYbdU"
// Authorized Email and Corresponding Password
#define USER_EMAIL "adrianxalzamora@gmail.com"
#define USER_PASSWORD "iot2023"

// network credentials 
#define DATABASE_URL "https://tastypoint-firebase-default-rtdb.firebaseio.com"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String tempPath = "/temperature";
String humPath = "/humidity";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;
int timestamp;

const char *ntpServer = "pool.ntp.org";

// Sensor
const int DHT_PIN = 15;
DHTesp dhtSensor;
float temperature;
float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 2000;

// Initialize WiFi
void initWiFi()
{
  Serial.print("Connecting Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return (0);
  }
  time(&now);
  return now;
}

// Write float values to the database
void sendFloat(String path, float value)
{
  if (Firebase.setFloat(fbdo, path.c_str(), value))
  {
    Serial.print("Writing value: ");
    Serial.print(value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  }
  else
  {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void setup(void)
{
  Serial.begin(115200);

  initWiFi();
  configTime(0, 0, ntpServer);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;

  // Since Firebase v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = String(auth.token.uid.c_str());
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/users/" + uid + "/readings";
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
}

void loop()
{

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    FirebaseJson json;
    // Get current timestamp
    timestamp = getTime();
    Serial.print("Time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);
    // Send readings to database:
    json.set(tempPath.c_str(), data.temperature);
    json.set(humPath.c_str(), data.humidity);
    json.set(timePath.c_str(), String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.setJSON(fbdo, parentPath.c_str(), json) ? "ok" : fbdo.errorReason().c_str());
  }
}

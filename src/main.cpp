#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ElegantOTA.h>
#include <WebSerial.h>
#include <ESPDash.h>

/* WIFI */
/* Your SoftAP WiFi Credentials */
const char *ssid = "testESP32";       // SSID
const char *password = "12345678"; // Password

/* Webserial Webserver */
AsyncWebServer serverSerial(80);
unsigned long last_print_time = millis();

/* OTA Webserver */
AsyncWebServer serverOTA(80);
unsigned long ota_progress_millis = 0;

/* ESPDASH Webserver */
AsyncWebServer serverDash(80);
ESPDash dashboard(&serverOTA);

Card temperature(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
Card humidity(&dashboard, HUMIDITY_CARD, "Humidity", "%");

long timer_dash = 0;
long timer_webserial = 0;
int count = 0;

void onOTAStart()
{
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final)
{
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000)
  {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success)
  {

    Serial.println("OTA update finished successfully!");
  }
  else
  {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void initWebserial()
{
  serverSerial.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(200, "text/plain", "Hi! This is WebSerial demo. You can access webserial interface at http://" + WiFi.softAPIP().toString() + "/webserial"); });

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&serverOTA);

  /* Attach Message Callback */
  WebSerial.onMessage([&](uint8_t *data, size_t len)
                      {
    Serial.printf("Received %u bytes from WebSerial: ", len);
    Serial.write(data, len);
    Serial.println();
    WebSerial.println("Received Data...");
    String d = "";
    for(size_t i=0; i < len; i++){
      d += char(data[i]);
    }
    WebSerial.println(d); });

  // Start server
  serverSerial.begin();
}

void initOTA()
{
  serverOTA.on("/up", HTTP_GET, [](AsyncWebServerRequest *request)
               { request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo."); });

  ElegantOTA.begin(&serverOTA); // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  ElegantOTA.setAutoReboot(true);

  serverOTA.begin();
  Serial.println("HTTP server started");
}

void initESPDash()
{
  serverDash.begin();
}

void setup()
{
  delay(3000);

  WiFi.mode(WIFI_AP);
  // WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  initOTA();
  initWebserial();
  initESPDash();

}

void loop()
{
  if (millis() - timer_dash > 1000)
  {
    temperature.update((int)random(0, 50));
    humidity.update((int)random(0, 100));

    /* Send Updates to our Dashboard (realtime) */
    dashboard.sendUpdates();
    timer_dash = millis();
  }

  if (millis() - timer_webserial > 2000) {
    count++;

    WebSerial.print(F("IP address: "));
    WebSerial.println(WiFi.softAPIP());
    WebSerial.printf("Uptime: %lums\n", millis());
    WebSerial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());

    timer_webserial = millis();
  }


  WebSerial.loop();
  ElegantOTA.loop();
}
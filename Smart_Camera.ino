#define CAMERA_MODEL_AI_THINKER
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include "camera_pins.hpp"
#include "camera_code.hpp"


// Wifi network station credentials
#define WIFI_SSID "MERCUSYS_687E"
#define WIFI_PASSWORD "0963948847"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "5414918739:AAEXCzMnYdlavY2ME0kVGl_OVpbpZhV-jBs"
void send_photo(String chat_id);
#define FLASH_LED_PIN 4
const int motionSensor = 13;
bool isDetected = false;
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
#define CHAT_ID "397362619"
unsigned long bot_lasttime; // last time messages' scan has been done
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

bool flashState = LOW;

camera_fb_t *fb = NULL;

bool isMoreDataAvailable();
byte *getNextBuffer();
int getNextBufferLen();

bool dataAvailable = false;

static void IRAM_ATTR detectsMovement(void * arg) {
  Serial.println("Motion Detected");
  isDetected = true;
}

void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/flash")
    {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    }

    if (text == "/photo")
    {
      send_photo(CHAT_ID);
    }

    if (text == "/start")
    {
      String welcome = "Welcome to the ESP32Cam Telegram bot.\n\n";
      welcome += "/photo : will take a photo\n";
      welcome += "/flash : toggle flash LED (VERY BRIGHT!)\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

bool isMoreDataAvailable()
{
  if (dataAvailable)
  {
    dataAvailable = false;
    return true;
  }
  else
  {
    return false;
  }
}

byte *getNextBuffer()
{
  if (fb)
  {
    return fb->buf;
  }
  else
  {
    return nullptr;
  }
}

int getNextBufferLen()
{
  if (fb)
  {
    return fb->len;
  }
  else
  {
    return 0;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  if (!setupCamera())
  {
    Serial.println("Camera Setup Failed!");
    while (true)
    {
      delay(100);
    }
  }
  
  // attempt to connect to Wifi network:
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  // Make the bot wait for a new message for up to 60seconds
  bot.longPoll = 60;
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState); //defaults to low
  pinMode(motionSensor, INPUT_PULLUP);

  auto err = gpio_isr_handler_add(GPIO_NUM_13, &detectsMovement, (void *) 13);
  if (err != ESP_OK) {
    Serial.printf("handler add failed with error 0x%x \r\n", err);
  }
  err = gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_POSEDGE);
  if (err != ESP_OK) {
    Serial.printf("set intr type failed with error 0x%x \r\n", err);
  }
}

void loop()
{
  /*
    if(isDetected){
      
      send_photo(CHAT_ID);
      isDetected = false;
      delay(3000);
    }
    Serial.println("Motion not Detected");
    delay(1000);
  //if (millis() - bot_lasttime > BOT_MTBS)
  //{
  */
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    delay(1000);
    bot_lasttime = millis();
    
}

void send_photo(String chat_id){
    fb = NULL;
    // Take Picture with Camera
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      bot.sendMessage(chat_id, "Camera capture failed", "");
      return;
    }
    dataAvailable = true;
    Serial.println("Sending");
    bot.sendPhotoByBinary(chat_id, "image/jpeg", fb->len,
                          isMoreDataAvailable, nullptr,
                          getNextBuffer, getNextBufferLen);

    Serial.println("done!");
    esp_camera_fb_return(fb);
}

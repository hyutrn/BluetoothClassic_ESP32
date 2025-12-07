#include <WiFi.h>
#include <PubSubClient.h>
#include <BluetoothSerial.h>


BluetoothSerial BT;
#define WIFI_SSID "Bap ngo"
#define WIFI_PASS "T35723511"
#define MQTT_BROKER "broker.mqtt-dashboard.com"
#define MQTT_PORT 1883
#define SUB_TOPIC "esp32/sub/topic"
#define PUB_TOPIC "esp32/pub/topic"

WiFiClient espClient;
PubSubClient client(espClient);

String btMessage = "";     // lưu tin nhắn nhận từ Bluetooth
bool hasMessage = false;   // cờ xác nhận có tin nhắn
enum Mode { BLUETOOTH_MODE, WIFI_MQTT_MODE };
Mode currentMode = BLUETOOTH_MODE;

// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.printf("Nội dung: %s\n", message);
}

void bluetoothMode() {
  Serial.println("Bluetooth Classic Mode:");
  Serial.println("Device name: ESP32_21521661");
  BT.begin("ESP32_21521661");

  while (currentMode == BLUETOOTH_MODE) {
    if (BT.available()) {
      btMessage = BT.readStringUntil('\n');
      btMessage.trim();
      Serial.print("Nhận Bluetooth: ");
      Serial.println(btMessage);

      hasMessage = true;
      currentMode = WIFI_MQTT_MODE;
      BT.end();
      delay(500);
      break;
    }
  }
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT connecting...");
    if (client.connect("ESP32Client")) {
      Serial.println("OK!");

      client.subscribe(SUB_TOPIC);
    } else {
      Serial.print("Failed, state = ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void wifiMqttMode() {
  Serial.println("=== WIFI + MQTT MODE ===");

  connectWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
  connectMQTT();

  // Gửi tin nhắn nhận được từ Bluetooth
  if (hasMessage) {
    Serial.print("Gửi MQTT: ");
    Serial.println(btMessage);

    if (client.publish(PUB_TOPIC, btMessage.c_str())) {
      Serial.println("Gửi MQTT thành công!");
    } else {
      Serial.println("Gửi MQTT thất bại!");
    }
  }

  // giữ kết nối vài giây (optional)
  unsigned long t = millis();
  while (millis() - t < 5000) {
    client.loop();
  }

  // Tắt WiFi và chuyển về Bluetooth
  Serial.println("Tắt WiFi...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  hasMessage = false;
  currentMode = BLUETOOTH_MODE;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Start");
}

void loop() {
  if (currentMode == BLUETOOTH_MODE) {
    bluetoothMode();
  }
  else if (currentMode == WIFI_MQTT_MODE) {
    wifiMqttMode();
  }
}


#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//Buzzer
const int pinBuzzer = 26;

//Variables Globales
String alarma = "unknown";
bool primer = true;

const char *WIFI_SSID = "UCB-PREMIUM";
const char *WIFI_PASS = "lacatoucb";

const char * MQTT_BROKER = "ab8zecc8ttmh2-ats.iot.us-east-2.amazonaws.com";
const int MQTT_BROKER_PORT = 8883;

const char * MQTT_CLIENT_ID = "adriel.mounzon@ucb.edu.bo";

const char * SUBSCRIBE_TOPIC_ALARMA = "estado_deseado_alarma";

const char AMAZON_ROOT_CA1[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
//Colocado en original
-----END CERTIFICATE-----
)EOF";

const char CERTIFICATE[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
//Colocado en original
-----END CERTIFICATE-----
)KEY";

const char PRIVATE_KEY[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
//Colocado en original
-----END RSA PRIVATE KEY-----
)KEY";

WiFiClientSecure wiFiClient;
PubSubClient mqttClient(wiFiClient);

StaticJsonDocument<JSON_OBJECT_SIZE(1)> inputDoc;

void callback(const char * topic, byte * payload, unsigned int lenght) {
  String message;
  for (int i = 0; i < lenght; i++) {
    message += String((char) payload[i]);
  }
  if (String(topic) == SUBSCRIBE_TOPIC_ALARMA) {
    Serial.println("Mensaje del topico " + String(topic) + ":" + message);

    DeserializationError err = deserializeJson(inputDoc, payload);
    if (!err) {
      String tmpEstadoAlarma = String(inputDoc["estado"].as<const char*>());
      if(!tmpEstadoAlarma.isEmpty() && (tmpEstadoAlarma.equals("activada") || tmpEstadoAlarma.equals("desactivada"))) 
      {
        alarma = tmpEstadoAlarma;
        Serial.print("Estado de la Alarma: ");
        Serial.println(alarma);
      }
    }
  }
}

boolean mqttClientConnect() {
  Serial.print("Conectando a " + String(MQTT_BROKER));
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println(" CONECTADO!");

    mqttClient.subscribe(SUBSCRIBE_TOPIC_ALARMA);
    Serial.println("Suscrito a " + String(SUBSCRIBE_TOPIC_ALARMA));
    
  } else {
    Serial.println("No se pudo conectar a " + String(MQTT_BROKER));
  }
  return mqttClient.connected();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Conectado a " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println(" CONECTADO!");

  wiFiClient.setCACert(AMAZON_ROOT_CA1);
  wiFiClient.setCertificate(CERTIFICATE);
  wiFiClient.setPrivateKey(PRIVATE_KEY);

  mqttClient.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
  mqttClient.setCallback(callback);
  pinMode(pinBuzzer, OUTPUT);
}

void loop() {

  Serial.println(alarma);
  if (!mqttClient.connected()) {
     mqttClientConnect();
   } else {
      if (alarma == "activada") {
        analogWrite(pinBuzzer, 255);
        delay(300);
        analogWrite(pinBuzzer, 0);
        delay(25);
      }
      else
      {
        if(primer==true)
        {
          alarma = "apagada";
          primer = false;
        }
      }
      mqttClient.loop();
  }
  delay(1000);
}
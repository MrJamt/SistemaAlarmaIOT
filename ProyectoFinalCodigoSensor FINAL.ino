#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//HC-SR04
int triggerPin = 14;
int echoPin = 12;

//HC-SR505
const int pinSensorMovimiento = 27;

//Variables Globales
String encendido = "unknown";
bool primer = true;


const char *WIFI_SSID = "UCB-PREMIUM";
const char *WIFI_PASS = "lacatoucb";

const char * MQTT_BROKER = "ab8zecc8ttmh2-ats.iot.us-east-2.amazonaws.com";
const int MQTT_BROKER_PORT = 8883;

const char * MQTT_CLIENT_ID = "jorge.montano@ucb.edu.bo";

const char * PUBLISH_TOPIC_HC505 = "jamt/output/hcm";

const char * SUBSCRIBE_TOPIC_SENSOR = "$aws/things/Mi_Objeto_IoT/shadow/name/shadow_alarma/update/accepted";

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

StaticJsonDocument<JSON_OBJECT_SIZE(3)> outputDoc;
char outputBuffer[128];

StaticJsonDocument<JSON_OBJECT_SIZE(64)> inputDoc;

void publishInfoSistema(int mov,int dist, String encendido){
  outputDoc["detectaMovimiento"] = mov;
  outputDoc["cambioDistancia"] = dist;
  outputDoc["estadoEncendido"] = encendido.c_str();
  serializeJson(outputDoc, outputBuffer);
  mqttClient.publish(PUBLISH_TOPIC_HC505, outputBuffer); 
}

void callback(const char * topic, byte * payload, unsigned int lenght) {
  String message;
  for (int i = 0; i < lenght; i++) {
    message += String((char) payload[i]);
  }
  if (String(topic) == SUBSCRIBE_TOPIC_SENSOR) {
    Serial.println("Mensaje del topico: " + String(topic) + ":" + message);

    DeserializationError err = deserializeJson(inputDoc, payload);
    if (!err) {
      String tmpestadoEncendidoReportado = String(inputDoc["state"]["reported"]["estadoEncendido"].as<const char*>());
      String tmpestadoEncendidoDeseado = String(inputDoc["state"]["desired"]["estadoEncendido"].as<const char*>());
      if(!tmpestadoEncendidoDeseado.isEmpty() && !tmpestadoEncendidoDeseado.equals(encendido)) 
      {
        encendido = tmpestadoEncendidoDeseado;
        publishInfoSistema(0,0,encendido);
        Serial.print("Estado de los Sensores: ");
        Serial.println(encendido);
      }
    }
  }
}

boolean mqttClientConnect() {
  Serial.print("Conectando a " + String(MQTT_BROKER));
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println(" CONECTADO!");

    mqttClient.subscribe(SUBSCRIBE_TOPIC_SENSOR);
    Serial.println("Suscrito a " + String(SUBSCRIBE_TOPIC_SENSOR));
    
  } else {
    Serial.println("No se pudo conectar a " + String(MQTT_BROKER));
  }
  return mqttClient.connected();
}

long readUltrasonicDistance(int triggerPin, int echoPin)
{
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  return pulseIn(echoPin, HIGH);
}

int detectaMovimiento()
{
  return digitalRead(pinSensorMovimiento);
}

float anteriorCm = 1000;

int cambioDistancia(){

  float cm = 0.01723 * readUltrasonicDistance(14, 12);

  if(anteriorCm!=1000)
  {
      if(anteriorCm-cm>20){
        anteriorCm = cm;
        return 1;
      }
  }
    anteriorCm = cm;
    return 0;
}

void imprimirMedidas(int mov, int dist)
{
  Serial.print("Cambio Distancia: ");
  Serial.print(dist);
  Serial.print(" | Cambio Movimiento: ");
  Serial.println(mov);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Conectando a " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println(" Conectado!");

  wiFiClient.setCACert(AMAZON_ROOT_CA1);
  wiFiClient.setCertificate(CERTIFICATE);
  wiFiClient.setPrivateKey(PRIVATE_KEY);

  mqttClient.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
  mqttClient.setCallback(callback);
  pinMode(pinSensorMovimiento, INPUT);
}

void loop() {

  Serial.println(encendido);

  if (!mqttClient.connected()) {
    mqttClientConnect();
  }
  else
  {
    mqttClient.loop();
    if(encendido=="encendido"){
      int mov = detectaMovimiento();
      int dist = cambioDistancia();
      imprimirMedidas(mov,dist); 
      if(mov==true || dist==true) {
        publishInfoSistema(mov,dist,encendido);
      }
    }
    else
    {
      if(primer==true)
      {
        encendido = "apagado";
        publishInfoSistema(0,0,encendido);
        primer=false;
      }
    }
  }
  delay(250);
}
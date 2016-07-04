#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

extern "C" {
#include "user_interface.h"
}

#define relaySolarPin D5
#define relayBoilerPin D6
#define relayBoostPin D7
#define ONE_WIRE_BUS D1
#define TUB_TEMP_SENSOR 0
#define PANEL_TEMP_SENSOR 1

int tempSolar = 0;
int tempTub = 0;
int tempTarget = 15;
int boostOn = LOW;
int boilerOn = LOW;
int solarOn = LOW;
unsigned long lastCheckTime;
unsigned long lastUpdateTime;

const char ssid[] = "granary";
const char password[] = "sparkym00se";
char myhostname[] = "HotTub";

WiFiServer server(80);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void checkTemps() {

  tempSolar = sensors.getTempCByIndex(0);
  tempTub = sensors.getTempCByIndex(1);
  Serial.print("Tub=");
  Serial.print(tempTub);
  Serial.print(" panel=");
  Serial.println(tempSolar);

  if (tempSolar > tempTub + 1)
    solarOn = HIGH;
  else if (tempSolar < tempTub - 1)
    solarOn = LOW;

  if (tempTub < tempTarget - 1)
    boilerOn = HIGH;
  else if (tempTub > tempTarget + 1) {
    boilerOn = LOW;
    boostOn = LOW;
  }

  digitalWrite(relaySolarPin, solarOn);
  digitalWrite(relayBoilerPin, boilerOn);
  digitalWrite(relayBoostPin, boostOn);

}

void setup() {

  pinMode(relaySolarPin, OUTPUT);
  pinMode(relayBoilerPin, OUTPUT);
  pinMode(relayBoostPin, OUTPUT);

  Serial.begin(115200);
  sensors.begin();
  delay(10);

  wifi_station_set_hostname(myhostname);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}

void loop() {
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (client) {

    // Wait until the client sends some data
    Serial.println("new client");
    while (!client.available()) {
      delay(1);
    }

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Match the request

    if (request.indexOf("/BOOST=ON") != -1) {
      boostOn = HIGH;
    }
    if (request.indexOf("/BOOST=OFF") != -1) {
      boostOn = LOW;
    }
    int pos = request.indexOf("/TARGET=");
    if ( pos != -1) {
      tempTarget = request.substring(pos + 8).toInt();
    }

    checkTemps();

    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta http-equiv=\"refresh\" content=\"5\" >");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Hot Tub</h1>");
    client.println("<table border=1>");
    client.print("<tr><td>Solar Panel Temp</td><td>");
    client.print(tempSolar);
    client.println("C</td></tr>");
    client.print("<tr><td>Tub Temperature</td><td>");
    client.print(tempTub);
    client.println("C</td></tr>");
    client.print("<tr><td>Target Temperature</td><td>");
    client.print("<a href=\"/TARGET=");
    client.print(tempTarget - 5);
    client.print("\">-</a> ");
    client.print(tempTarget);
    client.print("C <a href=\"/TARGET=");
    client.print(tempTarget + 5);
    client.println("\">+</a></td></tr>");
    client.print("<tr><td>Solar Panel Pump</td><td>");
    if (solarOn == HIGH) {
      client.print("On");
    }
    else {
      client.print("Off");
    }
    client.println("</td></tr>");
    client.print("<tr><td>Boiler</td><td>");
    if (boilerOn == HIGH) {
      client.print("On");
    }
    else {
      client.print("Off");
    }
    client.println("</td></tr>");
    client.print("<tr><td>Boost</td><td>");
    if (boostOn == HIGH) {
      client.print("<a href=\"/BOOST=OFF\">On</a>");
    }
    else {
      client.print("<a href=\"/BOOST=ON\">Off</a>");
    }
    client.println("</td></tr>");
    client.println("</table>");
    client.println("</body>");
    client.println("</html>");

    delay(1);
    Serial.println("Client disonnected");
    Serial.println("");
  }
  else {
    sensors.requestTemperatures();
    checkTemps();
  }

}


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
// Replace with your network credentials
const char* ssid = "wifi adı";
const char* password = "password";

WiFiServer server(80);

String header;
String r1State = "OFF";
String r2State = "OFF";
String r3State = "OFF";
const int r1 = 0;
const int r2 = 1;
const int r3 = 2;

unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

#define DHTPIN 2     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 5000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <title>ZAFER GUVEN - WIFI SICAKLIK & NEM OLCUMU</title>
  <h2>WIFI SICAKLIK & NEM OLCUMU</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#f93707;"></i> 
    <span class="dht-labels">SICAKLIK</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">NEM</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
  <h3>&copy; 2022 - ZAFER GUVEN - Kisisel Web Site - www.zaferguven.com</h3>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 5000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 5000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(t);
  }
  else if (var == "HUMIDITY") {
    return String(h);
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(r1, OUTPUT);
  pinMode(r2, OUTPUT);
  pinMode(r3, OUTPUT);
  digitalWrite(r1, LOW);
  digitalWrite(r2, LOW);
  digitalWrite(r3, LOW);
  dht.begin();
  Serial.println("AT+CWMODE=1");
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(h).c_str());
  });

  // Start server
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
  }

  WiFiClient client = server.available(); 

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();         
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            if (header.indexOf("GET /r1/off") >= 0) {
              Serial.println("r1 off");
              r1State = "OFF";
              digitalWrite(r1, LOW);
            }
            else if (header.indexOf("GET /r1/on") >= 0) {
              Serial.println("r1 on");
              r1State = "ON";
              digitalWrite(r1, HIGH);
            }
            else if (header.indexOf("GET /r2/off") >= 0) {
              Serial.println("r2 off");
              r2State = "OFF";
              digitalWrite(r2, LOW);
            }
            else if (header.indexOf("GET /r2/on") >= 0) {
              Serial.println("r2 on");
              r2State = "ON";
              digitalWrite(r2, HIGH);
            }
            else if (header.indexOf("GET /r3/off") >= 0) {
              Serial.println("r3 off");
              r3State = "OFF";
              digitalWrite(r3, LOW);
            }
            else if (header.indexOf("GET /r3/on") >= 0) {
              Serial.println("r3 on");
              r3State = "ON";
              digitalWrite(r3, HIGH);
            }
            else if (header.indexOf("GET /r2/off") >= 0) {
              Serial.println("r2 off");
              r2State = "OFF";
              digitalWrite(r2, LOW);
            }
            else if (header.indexOf("GET /r4/on") >= 0) {
              Serial.println("r4 on");
              r4State = "ON";
              digitalWrite(r4, HIGH);
            }
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            client.println("<title>ZAFER GUVEN - WIFI KONTROL SISTEMI</title>");
            client.println("<body><h1>WIFI ISI & NEM OLCUMU VE ROLE KONTROL SISTEMI</h1>");
            client.println("<br><h3>R1 DURUM = " + r1State + "</h3>");
            if (r1State=="OFF") {
              client.println("<p><a href=\"/r1/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/r1/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("<br><h3>R2 DURUM = " + r2State + "</h3>");
            if (r2State=="OFF") {
              client.println("<p><a href=\"/r2/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/r2/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("<br><h3>R3 DURUM = " + r3State + "</h3>");
            if (r3State=="OFF") {
              client.println("<p><a href=\"/r3/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/r3/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("<br><h3>&copy; 2022 - ZAFER GUVEN - Kisisel Web Site - www.zaferguven.com</h3>");
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

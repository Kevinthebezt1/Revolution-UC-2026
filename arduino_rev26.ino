#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "Donte";
const char* password = "abcdefg123";

WebServer server(80);
Servo myServo;

// =========================
// Servo config
// =========================
const int servoPin = 26;
const int closedAngle = 0;
const int openAngle = 90;
const unsigned long servoMoveDelay = 500;

// =========================
// State machine
// =========================
enum State {
  WAIT_CODE,
  OPENING,
  SESSION_ACTIVE,
  CLOSING
};

State currentState = WAIT_CODE;
unsigned long stateStartTime = 0;

// =========================
// Helpers
// =========================
String stateToString(State s) {
  switch (s) {
    case WAIT_CODE:      return "WAIT_CODE";
    case OPENING:        return "OPENING";
    case SESSION_ACTIVE: return "SESSION_ACTIVE";
    case CLOSING:        return "CLOSING";
    default:             return "UNKNOWN";
  }
}

void openServo() {
  myServo.write(openAngle);
  Serial.println("OK_OPEN");
}

void closeServo() {
  myServo.write(closedAngle);
  Serial.println("OK_CLOSE");
}

// =========================
// API handlers
// =========================
void handleRoot() {
  server.send(200, "text/plain", "ESP32 API READY");
}

void handleStatus() {
  String response = "";
  response += "STATE:";
  response += stateToString(currentState);
  response += "\nIP:";
  response += WiFi.localIP().toString();

  server.send(200, "text/plain", response);
}

void handleOpen() {
  if (currentState == WAIT_CODE) {
    stateStartTime = 0;
    currentState = OPENING;
    Serial.println("API_OPEN_ACCEPTED");
    server.send(200, "text/plain", "OPEN_ACCEPTED");
  } else {
    server.send(400, "text/plain", "ERR_BUSY");
  }
}

void handleClose() {
  currentState = CLOSING;
  stateStartTime = 0;
  Serial.println("API_CLOSE_ACCEPTED");
  server.send(200, "text/plain", "CLOSE_ACCEPTED");
}

void handleNotFound() {
  server.send(404, "text/plain", "NOT_FOUND");
}

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(closedAngle);

  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/open", handleOpen);
  server.on("/close", handleClose);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

// =========================
// Loop
// =========================
void loop() {
  server.handleClient();

  switch (currentState) {
    case WAIT_CODE:
      break;

    case OPENING:
      if (stateStartTime == 0) {
        openServo();
        stateStartTime = millis();
      }

      if (millis() - stateStartTime >= servoMoveDelay) {
        stateStartTime = 0;
        currentState = SESSION_ACTIVE;
        Serial.println("SESSION_STARTED");
      }
      break;

    case SESSION_ACTIVE:
      break;

    case CLOSING:
      closeServo();
      currentState = WAIT_CODE;
      Serial.println("SESSION_ENDED");
      break;
  }
}
// ESP32 profile commit marker

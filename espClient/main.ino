#define APP_DEBUG

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#include <utility>
#include <limits>

// Uncomment your board, or configure a custom board in Settings.h
#define USE_ESP32_DEV_MODULE
//#define USE_ESP32C3_DEV_MODULE
//#define USE_ESP32S2_DEV_KIT
//#define USE_WROVER_BOARD
//#define USE_TTGO_T7
//#define USE_TTGO_T_OI

const char* ssid = "swayam";
const char* password = "87655678";
const char* websocket_host = "192.168.159.122";
const uint16_t websocket_port = 6969;

WebSocketsClient webSocket;
bool connectedToServer = false;

StaticJsonDocument<512> voltammetryDoc;
StaticJsonDocument<256> messageDoc;

String serializeVoltammetryResults(double peakVoltage, double peakCurrent, const String& analyteName) {
    voltammetryDoc.clear();
    
    voltammetryDoc["type"] = "voltammetryResult";
    voltammetryDoc["analyte"] = analyteName;
    voltammetryDoc["peakVoltage"] = peakVoltage;
    voltammetryDoc["peakCurrent"] = peakCurrent;
    
    String output;
    serializeJson(voltammetryDoc, output);
    return output;
}

String serializeMessage(const String& type, const String& message) {
    messageDoc.clear();
    
    messageDoc["type"] = type;
    messageDoc["message"] = message;
    
    String output;
    serializeJson(messageDoc, output);
    return output;
}

void sendMessageOverSocket(const String& type, const String& message) {
    String jsonMessage = serializeMessage(type, message);
    webSocket.sendTXT(jsonMessage);
}

void sendVoltametryOverSocket(double peakVoltage, double peakCurrent, const String& analyteName) {
    String jsonMessage = serializeVoltammetryResults(peakVoltage, peakCurrent, analyteName);
    webSocket.sendTXT(jsonMessage);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            connectedToServer = true;
            Serial.println("Connected to WebSocket server");
            sendMessageOverSocket("init", "espInit");
            break;
            
        case WStype_DISCONNECTED:
            connectedToServer = false;
            Serial.println("Disconnected from WebSocket server");
            break;
            
        case WStype_TEXT:
            char* message = (char*) payload;
            Serial.print("Received: ");
            Serial.println(message);
            break;
    }
}

//1. Urea , 2. Uric Acid, 3. Creatin

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <vector>

using namespace std;

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_ADS1115 ads;

#include <SPI.h>

/*Parameters to be udated as per requirement*/
#define CYCLE_CNT 3
#define SLOPE_VAL 0.05
#define STEP_SIZE 0.01218
#define SWEEP_MODE 1

//DIFFERENCING OPERATION
#define vout2 1.995
#define T1 32
#define T2 25
#define T3 33

/* Sweep range for 1V P2P */
#define V0_5_UPPER_SWEEP 0
#define V0_5_LOWER_SWEEP 0
#define V0_5_P2P 1

/* Sweep range for 2V P2P */
#define V1_UPPER_SWEEP 3724
#define V1_LOWER_SWEEP 1241
#define V1_P2P 2

/* Setting the Sweep range as per Mode */
#define P2P (SWEEP_MODE == 1 ? V1_P2P : V0_5_P2P)
#define UPPER_SWEEP (SWEEP_MODE == 1 ? V1_UPPER_SWEEP : V0_5_UPPER_SWEEP)
#define LOWER_SWEEP (SWEEP_MODE == 1 ? V1_LOWER_SWEEP : V0_5_LOWER_SWEEP)

#define PROCESSING_LED 13
#define READY_LED 12
#define BUTTON 14

//INPUT FOR DAC
#define CS 26
#define SCK 18
#define MOSI 23
#define MCP4921_CONFIG 0x3000

//********************************GRAPHING PART***************************
#include "SPI.h"
#include "TFT_eSPI.h"

#define GRAPH_WIDTH 320
#define GRAPH_HEIGHT 240

uint16_t Scales_color = TFT_GREEN;
uint16_t Vals_color = TFT_RED;
uint16_t Points_color = TFT_CYAN;
uint16_t Axes_color = TFT_WHITE;

#define X_MIN -1.2
#define X_MAX 1.2
#define Y_MIN -60
#define Y_MAX 60

#define X_DIVISIONS 5
#define Y_DIVISIONS 5

std::vector<std::pair<double, double>> plotDataFor;
std::vector<std::pair<double, double>> plotDataBac;

int currentIndex = 0;
String prevNumber = "";
TFT_eSPI tft = TFT_eSPI();

int i;
float sampling_period;
int cycle;
float time_val;

SPIClass vspi(VSPI);

// System states
enum SystemState {
  IDLE,
  PROCESSING,
  COMPLETED
};

SystemState currentState = IDLE;
bool measurementStarted = false;
bool stopRequested = false;

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 500;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(15, 15);
  tft.print("Linear Sweep Voltammetry");

  Serial.begin(115200);
  Wire.setClock(400000);
  ads.begin();
  ads.setDataRate(RATE_ADS1115_860SPS);

  sampling_period = (P2P / (SLOPE_VAL * (V1_UPPER_SWEEP - V1_LOWER_SWEEP))) * 1000;

  vspi.begin(SCK, -1, MOSI, CS);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  pinMode(READY_LED, OUTPUT);
  pinMode(PROCESSING_LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(READY_LED, HIGH);
  digitalWrite(PROCESSING_LED, LOW);
  pinMode(T1, OUTPUT);
  pinMode(T2, OUTPUT);
  pinMode(T3, OUTPUT);
  digitalWrite(T1, HIGH);
  digitalWrite(T2, LOW);
  digitalWrite(T3, LOW);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.begin(websocket_host, websocket_port);
  webSocket.onEvent(webSocketEvent);
  
  // Display initial idle state
  displayIdleScreen();
}

void displayIdleScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(50, 100);
  tft.print("Press START to begin");
  tft.setCursor(50, 130);
  tft.print("LSV measurement");
}

void generateOutput(int forw) {
  int steps = 50;
  float sub_sampling_period = sampling_period / steps;
  float voltage_ave = 0;
  float current_ave = 0;
  
  for (int i = 1; i <= steps; i++) {
    // Check for stop request during measurement
    if (stopRequested) {
      return;
    }
    
    int16_t raw0 = ads.readADC_SingleEnded(0);
    int16_t raw1 = ads.readADC_SingleEnded(1);

    float voltage = raw0 * 0.1875 / 1000;
    float current = raw1 * 0.1875 / 1000;

    voltage_ave = (voltage + voltage_ave);
    current_ave = (current + current_ave);

    if (sub_sampling_period < 16) {
      delayMicroseconds(sub_sampling_period * 1000);
    } else {
      delay(sub_sampling_period);
    }
  }

  float actual_voltage = voltage_ave / steps;
  float actual_current = current_ave / steps;
  actual_voltage = actual_voltage - vout2;
  actual_current = (vout2 - actual_current) * 1000;

  time_val = time_val + sampling_period;

  plotPoint(actual_voltage, actual_current, Points_color);
  displayVals(actual_voltage, actual_current);
  
  if (forw == 1) {
    plotDataFor.push_back({ actual_voltage, actual_current });
  } else {
    plotDataBac.push_back({ actual_voltage, actual_current });
  }
}

void dacINPUT(uint16_t voltage) {
  voltage &= 0x0FFF;
  uint16_t command = (0x3000 | voltage);

  digitalWrite(CS, LOW);
  vspi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  vspi.transfer16(command);
  vspi.endTransaction();
  digitalWrite(CS, HIGH);
}

float X_AXIS_Y;
float Y_AXIS_X;

void drawGraphAxes() {
  X_AXIS_Y = map(0, Y_MIN, Y_MAX, GRAPH_HEIGHT - 10, 10);
  Y_AXIS_X = map(0, X_MIN, X_MAX, 10, GRAPH_WIDTH - 10);

  tft.drawLine(10, X_AXIS_Y, GRAPH_WIDTH - 10, X_AXIS_Y, Axes_color);
  tft.drawLine(Y_AXIS_X, 10, Y_AXIS_X, GRAPH_HEIGHT - 10, Axes_color);
  tft.setTextColor(Scales_color, TFT_BLACK);
  tft.setTextSize(1);

  for (int i = 0; i <= X_DIVISIONS; i++) {
    float xValue = X_MIN + (i * (X_MAX - X_MIN) / X_DIVISIONS);
    int xPixel = 10 + (i * (GRAPH_WIDTH - 20) / X_DIVISIONS);

    tft.drawLine(xPixel, X_AXIS_Y - 5, xPixel, X_AXIS_Y + 5, Scales_color);

    char label[10];
    sprintf(label, "%.1f V", xValue);
    tft.setCursor(xPixel - 10, X_AXIS_Y + 10);
    tft.print(label);
  }

  for (int i = 0; i <= Y_DIVISIONS; i++) {
    float yValue = Y_MIN + (i * (Y_MAX - Y_MIN) / Y_DIVISIONS);
    int yPixel = GRAPH_HEIGHT - 10 - (i * (GRAPH_HEIGHT - 20) / Y_DIVISIONS);

    tft.drawLine(Y_AXIS_X - 5, yPixel, Y_AXIS_X + 5, yPixel, TFT_WHITE);

    char label[10];
    sprintf(label, "%.1f uA", yValue);
    tft.setCursor(Y_AXIS_X - 25, yPixel - 5);
    tft.print(label);
  }
}

void plotPoint(float x, float y, uint16_t color) {
  float screenX = 10 + ((x - X_MIN) * (300) / (X_MAX - X_MIN));
  float screenY = 230 - ((y - Y_MIN) * (220) / (Y_MAX - Y_MIN));
  tft.fillCircle(screenX, screenY, 2, color);
}

void displayVals(float x, float y) {
  tft.setTextSize(2);

  int16_t width = tft.textWidth(String(prevNumber));
  tft.fillRect(20, 210, width, tft.fontHeight(), TFT_BLACK);

  tft.setTextColor(Vals_color, TFT_BLACK);
  tft.setCursor(20, 210);
  tft.print(String(x, 1) + " V," + String(y, 1) + " uA, CYCLE =" + String(cycle));

  prevNumber = String(x, 1) + " V," + String(y, 1) + " uA, CYCLE =" + String(cycle);
  tft.setTextSize(2);
}

void findLocalMaxima(const std::vector<std::pair<double, double>>& coordinates, std::vector<std::pair<double, double>>& maxima) {
  int size = coordinates.size();
  for (int i = 1; i < size - 1; i++) {
    double x_prev = coordinates[i - 1].first, y_prev = coordinates[i - 1].second;
    double x_curr = coordinates[i].first, y_curr = coordinates[i].second;
    double x_next = coordinates[i + 1].first, y_next = coordinates[i + 1].second;

    double slope1 = (y_curr - y_prev) / (x_curr - x_prev);
    double slope2 = (y_next - y_curr) / (x_next - x_curr);

    if (slope1 >= 0 && slope2 <= 0) {
      maxima.push_back({ x_curr, y_curr });
    }
  }
}

void findLocalMinima(const std::vector<std::pair<double, double>>& coordinates, std::vector<std::pair<double, double>>& minima) {
  int size = coordinates.size();
  for (int i = 1; i < size - 1; i++) {
    double x_prev = coordinates[i - 1].first, y_prev = coordinates[i - 1].second;
    double x_curr = coordinates[i].first, y_curr = coordinates[i].second;
    double x_next = coordinates[i + 1].first, y_next = coordinates[i + 1].second;

    double slope1 = (y_curr - y_prev) / (x_curr - x_prev);
    double slope2 = (y_next - y_curr) / (x_next - x_curr);

    if (slope1 >= 0 && slope2 <= 0) {
      minima.push_back({ x_curr, y_curr });
    }
  }
}

std::pair<double, double> findGlobalMaximum(const std::vector<std::pair<double, double>>& maxima) {
  std::pair<double, double> globalMax = { 0, numeric_limits<double>::lowest() };
  for (const auto& point : maxima) {
    if (point.second > globalMax.second) {
      globalMax = point;
    }
  }
  return globalMax;
}

std::pair<double, double> findGlobalMinimum(const std::vector<std::pair<double, double>>& minima) {
  std::pair<double, double> globalMin = { 0, numeric_limits<double>::max() };
  for (const auto& point : minima) {
    if (point.second < globalMin.second) {
      globalMin = point;
    }
  }
  return globalMin;
}

void switchElectrode(int cycleNum) {
  if (cycleNum == 1) {
    digitalWrite(T2, LOW);
    digitalWrite(T1, HIGH);
    digitalWrite(T3, LOW);
  } else if (cycleNum == 2) {
    digitalWrite(T1, LOW);
    digitalWrite(T3, LOW);
    digitalWrite(T2, HIGH);
  } else if (cycleNum == 3) {
    digitalWrite(T1, LOW);
    digitalWrite(T2, LOW);
    digitalWrite(T3, HIGH);
  }
}

void handleButtons() {
  unsigned long currentTime = millis();
  
  // Handle single button - START when idle, STOP when processing
  if (digitalRead(BUTTON) == LOW) {
    if (currentTime - lastButtonPress > debounceDelay) {
      lastButtonPress = currentTime;
      
      if (currentState == IDLE) {
        // START measurement
        Serial.println("START button pressed - Beginning measurement");
        currentState = PROCESSING;
        measurementStarted = true;
        stopRequested = false;
        cycle = 1;
        
        digitalWrite(READY_LED, LOW);
        digitalWrite(PROCESSING_LED, HIGH);
        
        // Initialize display
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setCursor(5, 50);
        tft.print("LSV electrode " + String(cycle));
        
        // Prepare for measurement
        tft.setTextSize(2);
        tft.fillScreen(TFT_BLACK);
        drawGraphAxes();
        time_val = 0;
        
        switchElectrode(cycle);
      } else if (currentState == PROCESSING) {
        // STOP measurement
        Serial.println("STOP button pressed - Stopping measurement");
        
        currentState = IDLE;
        measurementStarted = false;
        stopRequested = true;
        cycle = 1;
        
        digitalWrite(READY_LED, HIGH);
        digitalWrite(PROCESSING_LED, LOW);
        
        // Clear data
        plotDataFor.clear();
        plotDataBac.clear();
        
        // Show idle screen
        displayIdleScreen();
      }

      String str;
      switch(currentState) {
          case IDLE: str = "IDLE"; break;
          case PROCESSING: str = "PROCESSING"; break;
          case COMPLETED: str = "COMPLETED"; break;
      }

      sendMessageOverSocket("processingState", str);
    }
  }
}

void loop() {
  webSocket.loop();
  handleButtons();
  
  // Only process if we're in PROCESSING state and measurement started
  if (currentState == PROCESSING && measurementStarted && !stopRequested) {
    
    // Perform measurement for current cycle
    if (cycle <= CYCLE_CNT) {
      i = LOWER_SWEEP;
      
      while (i <= UPPER_SWEEP && !stopRequested) {
        webSocket.loop();
        handleButtons(); // Check for stop during measurement
        
        if (stopRequested) {
          break;
        }
        
        dacINPUT(i);
        i += 25;
        generateOutput(1);
      }
      
      // If stop was requested during measurement, don't process results
      if (stopRequested) {
        return;
      }
      
      // Process results
      std::vector<std::pair<double, double>> maxima;
      std::vector<std::pair<double, double>> minima;
      findLocalMaxima(plotDataFor, maxima);
      findLocalMinima(plotDataBac, minima);
      
      std::pair<double, double> globalMax = findGlobalMaximum(maxima);
      std::pair<double, double> globalMin = findGlobalMinimum(minima);
      
      // Send results
      String analyteName;
      if (cycle == 1) {
        analyteName = "Urea";
      } else if (cycle == 2) {
        analyteName = "Uric Acid";
      } else if (cycle == 3) {
        analyteName = "Creatinine";
      } else {
        analyteName = "Unknown";
      }
      
      sendVoltametryOverSocket(globalMax.first, globalMax.second, analyteName);
      
      // Show results
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(5, 50);
      tft.print("LSV Completed, electrode =" + String(cycle));
      
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(5, 50);
      tft.print(" Max Current : ");
      tft.print(globalMax.second);
      tft.print(" at ");
      tft.print(globalMax.first);
      
      // Clean up
      plotDataFor.clear();
      plotDataBac.clear();
      
      cycle++;
      
      if (cycle <= CYCLE_CNT) {
        // Prepare for next cycle
        switchElectrode(cycle);
        
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setCursor(5, 50);
        tft.print("LSV electrode " + String(cycle));
        
        tft.setTextSize(2);
        tft.fillScreen(TFT_BLACK);
        drawGraphAxes();
      } else {
        // All cycles completed
        currentState = COMPLETED;
        measurementStarted = false;
        digitalWrite(READY_LED, HIGH);
        digitalWrite(PROCESSING_LED, LOW);
        
        // Show completion screen
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(30, 100);
        tft.print("All measurements");
        tft.setCursor(30, 130);
        tft.print("completed!");
        tft.setCursor(30, 160);
        tft.print("Press START for new");
        tft.setCursor(30, 190);
        tft.print("measurement");
        
        // Reset to idle after showing completion
        delay(3000);
        currentState = IDLE;
        displayIdleScreen();
      }
    }
  }
}
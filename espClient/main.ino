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

// Unique device identifier — generated from MAC address
String DEVICE_ID = "ESP32_UNKNOWN";

const char* websocket_host = "renalhealthmonitor.onrender.com";
const uint16_t websocket_port = 443;

WebSocketsClient webSocket;
bool connectedToServer = false;

StaticJsonDocument<512> voltammetryDoc;
StaticJsonDocument<256> messageDoc;

// Calibration constants for each analyte
// Urea calibration
const float C_UREA = 10;
const float M_UREA = 2;

// Uric Acid calibration
const float C_URIC_ACID = 8;
const float M_URIC_ACID = 1.5;

// Creatinine calibration
const float C_CREATININE = 12;
const float M_CREATININE = 2.5;

String serializeVoltammetryResults(double concentration, double peakCurrent, const String& analyteName) {
    voltammetryDoc.clear();
    
    voltammetryDoc["type"] = "voltammetryResult";
    voltammetryDoc["deviceId"] = DEVICE_ID;
    voltammetryDoc["analyte"] = analyteName;
    voltammetryDoc["concentration"] = concentration;
    voltammetryDoc["peakCurrent"] = peakCurrent;
    
    String output;
    serializeJson(voltammetryDoc, output);
    return output;
}

String serializeMessage(const String& type, const String& message) {
    messageDoc.clear();
    
    messageDoc["type"] = type;
    messageDoc["deviceId"] = DEVICE_ID;
    messageDoc["data"] = message;
    
    String output;
    serializeJson(messageDoc, output);
    return output;
}

void sendMessageOverSocket(const String& type, const String& message) {
    String jsonMessage = serializeMessage(type, message);
    webSocket.sendTXT(jsonMessage);
}

void sendVoltammetryOverSocket(double concentration, double peakCurrent, const String& analyteName) {
    String jsonMessage = serializeVoltammetryResults(concentration, peakCurrent, analyteName);
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

// Graph axis ranges — mutable for auto-scaling
float graphXMin = -1.2;
float graphXMax = 1.2;
float graphYMin = -60;
float graphYMax = 60;

// Initial default ranges (used to reset before each cycle)
const float DEFAULT_X_MIN = -1.2;
const float DEFAULT_X_MAX = 1.2;
const float DEFAULT_Y_MIN = -60;
const float DEFAULT_Y_MAX = 60;

#define X_DIVISIONS 5
#define Y_DIVISIONS 5

// Graph drawing area with margins
#define GRAPH_LEFT   35
#define GRAPH_RIGHT  (GRAPH_WIDTH - 5)
#define GRAPH_TOP    5
#define GRAPH_BOTTOM (GRAPH_HEIGHT - 30)

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

// Timer for COMPLETED state
unsigned long completionTime = 0;

// Store concentration results for each cycle to show on completion screen
float cycleConcentrations[CYCLE_CNT] = {0};
const char* cycleAnalyteNames[CYCLE_CNT] = {"Urea", "Uric Acid", "Creatinine"};

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(15, 30);
  tft.print("Renal Health Monitor");
  tft.setTextSize(1);
  tft.setCursor(15, 55);
  tft.print("Initializing...");

  Serial.begin(115200);

  // Generate unique device ID from MAC address
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  DEVICE_ID = "ESP32_" + mac;
  Serial.println("Device ID: " + DEVICE_ID);

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

  webSocket.beginSSL(websocket_host, websocket_port);
  webSocket.onEvent(webSocketEvent);
  
  // Display initial idle state
  displayIdleScreen();
}

void displayIdleScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 80);
  tft.print("Renal Health Monitor");
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 110);
  tft.print("Press START to begin measurement");
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
  float plotW = GRAPH_RIGHT - GRAPH_LEFT;
  float plotH = GRAPH_BOTTOM - GRAPH_TOP;

  // Compute pixel positions for the zero axes
  X_AXIS_Y = GRAPH_BOTTOM - ((0 - graphYMin) / (graphYMax - graphYMin)) * plotH;
  Y_AXIS_X = GRAPH_LEFT + ((0 - graphXMin) / (graphXMax - graphXMin)) * plotW;

  // Clamp axis lines to graph area
  X_AXIS_Y = constrain(X_AXIS_Y, GRAPH_TOP, GRAPH_BOTTOM);
  Y_AXIS_X = constrain(Y_AXIS_X, GRAPH_LEFT, GRAPH_RIGHT);

  tft.drawLine(GRAPH_LEFT, X_AXIS_Y, GRAPH_RIGHT, X_AXIS_Y, Axes_color);
  tft.drawLine(Y_AXIS_X, GRAPH_TOP, Y_AXIS_X, GRAPH_BOTTOM, Axes_color);
  tft.setTextColor(Scales_color, TFT_BLACK);
  tft.setTextSize(1);

  for (int i = 0; i <= X_DIVISIONS; i++) {
    float xValue = graphXMin + (i * (graphXMax - graphXMin) / X_DIVISIONS);
    int xPixel = GRAPH_LEFT + (i * plotW / X_DIVISIONS);

    tft.drawLine(xPixel, X_AXIS_Y - 3, xPixel, X_AXIS_Y + 3, Scales_color);

    char label[10];
    sprintf(label, "%.1f", xValue);
    tft.setCursor(xPixel - 8, GRAPH_BOTTOM + 4);
    tft.print(label);
  }

  for (int i = 0; i <= Y_DIVISIONS; i++) {
    float yValue = graphYMin + (i * (graphYMax - graphYMin) / Y_DIVISIONS);
    int yPixel = GRAPH_BOTTOM - (i * plotH / Y_DIVISIONS);

    tft.drawLine(Y_AXIS_X - 3, yPixel, Y_AXIS_X + 3, yPixel, TFT_WHITE);

    char label[10];
    sprintf(label, "%.0f", yValue);
    tft.setCursor(2, yPixel - 4);
    tft.print(label);
  }
}

void plotPoint(float x, float y, uint16_t color) {
  float plotW = GRAPH_RIGHT - GRAPH_LEFT;
  float plotH = GRAPH_BOTTOM - GRAPH_TOP;

  float screenX = GRAPH_LEFT + ((x - graphXMin) * plotW / (graphXMax - graphXMin));
  float screenY = GRAPH_BOTTOM - ((y - graphYMin) * plotH / (graphYMax - graphYMin));

  // Clamp to graph area so points never draw outside the axes
  screenX = constrain(screenX, GRAPH_LEFT, GRAPH_RIGHT);
  screenY = constrain(screenY, GRAPH_TOP, GRAPH_BOTTOM);

  tft.fillCircle((int)screenX, (int)screenY, 2, color);
}

void displayVals(float x, float y) {
  tft.setTextSize(1);

  int16_t width = tft.textWidth(String(prevNumber));
  tft.fillRect(GRAPH_LEFT, GRAPH_BOTTOM + 14, GRAPH_RIGHT - GRAPH_LEFT, tft.fontHeight(), TFT_BLACK);

  tft.setTextColor(Vals_color, TFT_BLACK);
  tft.setCursor(GRAPH_LEFT, GRAPH_BOTTOM + 14);
  String info = String(x, 2) + "V  " + String(y, 1) + "uA  E" + String(cycle);
  tft.print(info);

  prevNumber = info;
}

void findLocalMaxima(const std::vector<std::pair<double, double>>& coordinates, std::vector<std::pair<double, double>>& maxima) {
  int size = coordinates.size();
  for (int i = 1; i < size - 1; i++) {
    double x_prev = coordinates[i - 1].first, y_prev = coordinates[i - 1].second;
    double x_curr = coordinates[i].first, y_curr = coordinates[i].second;
    double x_next = coordinates[i + 1].first, y_next = coordinates[i + 1].second;

    double dx1 = x_curr - x_prev;
    double dx2 = x_next - x_curr;
    if (dx1 == 0 || dx2 == 0) continue; // skip to avoid division by zero

    double slope1 = (y_curr - y_prev) / dx1;
    double slope2 = (y_next - y_curr) / dx2;

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

    double dx1 = x_curr - x_prev;
    double dx2 = x_next - x_curr;
    if (dx1 == 0 || dx2 == 0) continue; // skip to avoid division by zero

    double slope1 = (y_curr - y_prev) / dx1;
    double slope2 = (y_next - y_curr) / dx2;

    if (slope1 <= 0 && slope2 >= 0) {
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

// Function to calculate concentration based on analyte type
float calculateConcentration(double peakCurrent, int cycleNum) {
  float conc;
  
  if (cycleNum == 1) {
    // Urea
    conc = (peakCurrent - C_UREA) / M_UREA;
  } else if (cycleNum == 2) {
    // Uric Acid
    conc = (peakCurrent - C_URIC_ACID) / M_URIC_ACID;
  } else if (cycleNum == 3) {
    // Creatinine
    conc = (peakCurrent - C_CREATININE) / M_CREATININE;
  } else {
    // Default fallback (shouldn't reach here)
    conc = 0;
  }
  
  return conc;
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
        // Reset stored concentrations
        for (int c = 0; c < CYCLE_CNT; c++) cycleConcentrations[c] = 0;
        
        digitalWrite(READY_LED, LOW);
        digitalWrite(PROCESSING_LED, HIGH);
        
        // Initialize display
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setCursor(5, 50);
        tft.print("Electrode " + String(cycle) + ": " + String(cycleAnalyteNames[cycle - 1]));
        
        // Reset graph ranges for new measurement
        graphXMin = DEFAULT_X_MIN; graphXMax = DEFAULT_X_MAX;
        graphYMin = DEFAULT_Y_MIN; graphYMax = DEFAULT_Y_MAX;
        
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
        for (int c = 0; c < CYCLE_CNT; c++) cycleConcentrations[c] = 0;
        
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
      
      if (!maxima.empty()) {
        std::pair<double, double> globalMax = findGlobalMaximum(maxima);
        // Calculate concentration using analyte-specific calibration
        float conc = calculateConcentration(globalMax.second, cycle);
        cycleConcentrations[cycle - 1] = conc;
        sendVoltammetryOverSocket(conc, globalMax.second, analyteName);
      
        // Show per-cycle results briefly
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(5, 40);
        tft.print(analyteName + " done");
        tft.setCursor(5, 70);
        tft.print("Peak: " + String(globalMax.second, 2) + " uA");
        tft.setCursor(5, 100);
        tft.print("Conc: " + String(conc, 2));
      } else {
        Serial.println("Warning: No peaks found for " + analyteName);
      }
      
      // Clean up
      plotDataFor.clear();
      plotDataBac.clear();
      
      cycle++;
      
      if (cycle <= CYCLE_CNT) {
        // Prepare for next cycle
        switchElectrode(cycle);
        
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setCursor(5, 50);
        tft.print("Electrode " + String(cycle) + ": " + String(cycleAnalyteNames[cycle - 1]));
        
        // Reset graph ranges for next cycle
        graphXMin = DEFAULT_X_MIN; graphXMax = DEFAULT_X_MAX;
        graphYMin = DEFAULT_Y_MIN; graphYMax = DEFAULT_Y_MAX;
        
        tft.setTextSize(2);
        tft.fillScreen(TFT_BLACK);
        drawGraphAxes();
      } else {
        // All cycles completed
        currentState = COMPLETED;
        measurementStarted = false;
        digitalWrite(READY_LED, HIGH);
        digitalWrite(PROCESSING_LED, LOW);
        
        // Show completion screen with concentrations
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(20, 10);
        tft.print("Test Complete!");
        
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int yPos = 45;
        for (int c = 0; c < CYCLE_CNT; c++) {
          tft.setTextColor(TFT_CYAN, TFT_BLACK);
          tft.setCursor(10, yPos);
          tft.print(String(cycleAnalyteNames[c]) + ":");
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setCursor(160, yPos);
          if (cycleConcentrations[c] != 0) {
            tft.print(String(cycleConcentrations[c], 3));
            tft.print(c == 0 ? " mM" : " uM");
          } else {
            tft.print("N/A");
          }
          yPos += 30;
        }
        
        tft.setTextSize(1);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(20, yPos + 20);
        tft.print("Auto-reset in 10s...");
  
        completionTime = millis();
        sendMessageOverSocket("processingState", "COMPLETED");
      }
    }
  }

  if (currentState == COMPLETED) {
    // Check if 3 seconds (3000 ms) have passed
    if (millis() - completionTime > 10000) {
      currentState = IDLE;
      displayIdleScreen();
      sendMessageOverSocket("processingState", "IDLE");
    }
  }
}

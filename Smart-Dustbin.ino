/********** Libraries **********/
#define BLYNK_TEMPLATE_ID "TMPL3b-KK6ipq"
#define BLYNK_TEMPLATE_NAME "Smart Waste Segregation"
#define BLYNK_DEVICE_NAME "SmartWasteSegregator"
#define BLYNK_AUTH_TOKEN "G8RnVxxjAkOOr2jQEFTOx04_ilhx2Thd"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

/********** OLED Setup **********/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/********** WiFi & Blynk **********/
char ssid[] = "*****";
char pass[] = "16052006";

/********** Blynk Virtual Pins **********/
#define VP_BIN_STATUS      V0
#define VP_WASTE_DETECTED  V1
#define VP_WASTE_TYPE      V2
#define VP_PLASTIC_LEVEL   V3
#define VP_ORGANIC_LEVEL   V4
#define VP_METAL_LEVEL     V5
#define MOISTURE_VPIN      V6
#define VP_SYSTEM_LOG      V7
#define VP_BATTERY_MONITOR V8

/********** Sensor Pins **********/
// Waste detection
#define IR_SENSOR_PIN   34
#define ORGANIC_PIN     35  
#define METAL_PIN       32  
#define PLASTIC_PIN     33  

#define ORGANIC_THRESHOLD 600

// Ultrasonic sensors
#define PLASTIC_TRIG 12
#define PLASTIC_ECHO 14
#define ORGANIC_TRIG 26
#define ORGANIC_ECHO 27
#define METAL_TRIG   25
#define METAL_ECHO   13

// LEDs
#define PLASTIC_LED 18
#define ORGANIC_LED 19
#define METAL_LED    5

// Servos
#define SERVO_INIT_PIN  15
#define SERVO_SORT_PIN  17
#define SERVO_EXTRA_PIN 16

// Buzzer
#define BUZZER_PIN 2

/********** Servo Objects **********/
Servo servoInit;
Servo servoSort;
Servo servoExtra;

/********** Functions **********/
long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(ORGANIC_PIN, INPUT);
  pinMode(METAL_PIN, INPUT);
  pinMode(PLASTIC_PIN, INPUT);

  pinMode(PLASTIC_TRIG, OUTPUT);
  pinMode(PLASTIC_ECHO, INPUT);
  pinMode(ORGANIC_TRIG, OUTPUT);
  pinMode(ORGANIC_ECHO, INPUT);
  pinMode(METAL_TRIG, OUTPUT);
  pinMode(METAL_ECHO, INPUT);

  pinMode(PLASTIC_LED, OUTPUT);
  pinMode(ORGANIC_LED, OUTPUT);
  pinMode(METAL_LED, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  servoInit.attach(SERVO_INIT_PIN);
  servoSort.attach(SERVO_SORT_PIN);
  servoExtra.attach(SERVO_EXTRA_PIN);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Ready");
  display.display();
  delay(1500);

  Blynk.begin(ssid, pass);
}

void loop() {
  Blynk.run();

  display.clearDisplay();
  display.setCursor(0, 0);

  if (digitalRead(IR_SENSOR_PIN) == LOW) {
    display.println("Waste Detected");
    display.display();
    Blynk.virtualWrite(VP_WASTE_DETECTED, 1);
    delay(500);

    // Metal detection
    if (digitalRead(METAL_PIN) == HIGH) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Type: Metal");
      display.display();
      digitalWrite(METAL_LED, HIGH);
      Blynk.virtualWrite(VP_WASTE_TYPE, "Metal");
      tone(BUZZER_PIN, 1000, 300);
      servoSort.write(90);
      delay(1000);
      digitalWrite(METAL_LED, LOW);
    }
    // Plastic detection
    else if (digitalRead(PLASTIC_PIN) == HIGH) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Type: Plastic");
      display.display();
      digitalWrite(PLASTIC_LED, HIGH);
      Blynk.virtualWrite(VP_WASTE_TYPE, "Plastic");
      tone(BUZZER_PIN, 1200, 300);
      servoSort.write(45);
      delay(1000);
      digitalWrite(PLASTIC_LED, LOW);
    }
    // Organic detection
    else {
      int organicSum = 0;
      for (int i = 0; i < 5; i++) {
        organicSum += analogRead(ORGANIC_PIN);
        delay(10);
      }
      int organicAvg = organicSum / 5;
      Blynk.virtualWrite(MOISTURE_VPIN, organicAvg);

      if (organicAvg > ORGANIC_THRESHOLD) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Type: Organic");
        display.display();
        digitalWrite(ORGANIC_LED, HIGH);
        Blynk.virtualWrite(VP_WASTE_TYPE, "Organic");
        tone(BUZZER_PIN, 800, 300);
        servoSort.write(135);
        delay(1000);
        digitalWrite(ORGANIC_LED, LOW);
      } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Type: Unknown");
        display.display();
        Blynk.virtualWrite(VP_WASTE_TYPE, "Unknown");
      }
    }
  } else {
    display.println("No Waste");
    display.display();
    Blynk.virtualWrite(VP_WASTE_DETECTED, 0);
  }

  // Bin level monitoring
  int plasticLevel = getDistance(PLASTIC_TRIG, PLASTIC_ECHO);
  int organicLevel = getDistance(ORGANIC_TRIG, ORGANIC_ECHO);
  int metalLevel   = getDistance(METAL_TRIG, METAL_ECHO);

  Blynk.virtualWrite(VP_PLASTIC_LEVEL, plasticLevel);
  Blynk.virtualWrite(VP_ORGANIC_LEVEL, organicLevel);
  Blynk.virtualWrite(VP_METAL_LEVEL, metalLevel);

  delay(300);
}
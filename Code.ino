/********************  Blynk credentials  ***************************/
#define BLYNK_TEMPLATE_ID "TMPL3RRosQO72"
#define BLYNK_TEMPLATE_NAME "MARK1"
#define BLYNK_AUTH_TOKEN "Dq8wWzHd8HNMzghNqOWymlCi9PWJKTNC"

char ssid[]     = "toxic33";
char pass[]     = "0987654321";
/*******************************************************************/

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

/* ---------- Pin Map (unchanged) --------------------------------*/
// Motor A
#define IN1 13
#define IN2 14
#define ENA 12     // PWM (ledcAttach)

// Motor B
#define IN3 27
#define IN4 26
#define ENB 25     // PWM (ledcAttach)

// Inputs
#define POT1_PIN 34
#define POT2_PIN 33
#define IR_PIN   18
#define LM35_PIN 36

// Relays – active-LOW
#define RELAY1_PIN 16      // trips on over-current
#define RELAY2_PIN 17      // trips on over-temperature / IR-clear

/* ---------- Protection thresholds ------------------------------*/
const float V_DC            = 12.0;   // motor supply
const float V_AC            = 12.0;   // transformer secondary
const float MOTOR_IMPEDANCE = 13.0;   // Ω per motor
const float EFFICIENCY      = 0.85;
const float AC_LIMIT_A      = 1.5;    // relay-1 trips ≥ 1.5 A
const float TEMP_LIMIT_C    = 50.0;   // relay-2 trips ≥ 50 °C

/* ---------- Global data for dashboard --------------------------*/
float gTempC = 0.0;
float gIac   = 0.0;
bool  gIrBlk = false;        // TRUE = obstacle present

/* ---------- Manual relay wishes (dashboard switches) -----------*/
bool relay1WishOn = false;   // user wants ON
bool relay2WishOn = false;

/* ---------- Blynk virtual pins --------------------------------*/
#define VP_RELAY1 V0
#define VP_RELAY2 V1
#define VP_TEMP   V2
#define VP_IAC    V3
#define VP_IR     V4

BlynkTimer timer;

/*** Blynk handlers – save user wishes ****************************/
BLYNK_WRITE(VP_RELAY1) { relay1WishOn = param.asInt(); }
BLYNK_WRITE(VP_RELAY2) { relay2WishOn = param.asInt(); }

/*** Push live data to dashboard **********************************/
void sendSensorData() {
  Blynk.virtualWrite(VP_TEMP, gTempC);
  Blynk.virtualWrite(VP_IAC,  gIac);
  Blynk.virtualWrite(VP_IR,   gIrBlk ? 1 : 0);
}

/******************************************************************/
void setup() {
  Serial.begin(115200);

  /* GPIO directions */
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // default forward
  digitalWrite(IN1, HIGH);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);   // HIGH = relay OFF (idle)
  digitalWrite(RELAY2_PIN, HIGH);

  pinMode(IR_PIN, INPUT_PULLUP);

  /* PWM attach – one-liner */
  ledcAttach(ENA, 1000, 8);   // motor A
  ledcAttach(ENB, 1000, 8);   // motor B

  /* Blynk */
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, sendSensorData);   // push once / sec
}

/******************************************************************/
void loop() {
  /**** 1 – Read sensors *******************************************/
  int pot1Raw   = analogRead(POT1_PIN);     // 0-4095
  int pot2Raw   = analogRead(POT2_PIN);
  int lm35Raw   = analogRead(LM35_PIN);
  bool obstacle = (digitalRead(IR_PIN) == LOW);   // LOW = blocked

  float tempC = (lm35Raw / 4095.0f) * 3.3f * 100.0f;

  /**** 2 – Map pots directly to PWM (motors never stop) ***********/
  int pwmA = map(pot1Raw, 0, 4095, 0, 255);
  int pwmB = map(pot2Raw, 0, 4095, 0, 255);

  float dutyA = pwmA / 255.0f;
  float dutyB = pwmB / 255.0f;

  /**** 3 – Estimate total AC current *****************************/
  float I1_DC     = (V_DC * dutyA) / MOTOR_IMPEDANCE;
  float I2_DC     = (V_DC * dutyB) / MOTOR_IMPEDANCE;
  float I_totalDC = I1_DC + I2_DC;
  float I_AC      = (I_totalDC * V_DC) / (V_AC * EFFICIENCY);

  bool overCurrent = (I_AC >= AC_LIMIT_A);
  bool overTemp    = (tempC >= TEMP_LIMIT_C);

  /**** 4 – Decide final relay ON/OFF *****************************/
  // Relay-1: trips on over-current only
  bool relay1ShouldBeOn = relay1WishOn && !overCurrent;

  // Relay-2: trips on over-temperature OR when IR beam is clear
  // obstacle == false means IR is clear
  bool relay2ShouldBeOn = relay2WishOn && !overTemp && obstacle;

  // convert to pin state (active-LOW)
  digitalWrite(RELAY1_PIN, relay1ShouldBeOn ? LOW : HIGH);
  digitalWrite(RELAY2_PIN, relay2ShouldBeOn ? LOW : HIGH);

  // keep dashboard switches in sync
  Blynk.virtualWrite(VP_RELAY1, relay1ShouldBeOn ? 1 : 0);
  Blynk.virtualWrite(VP_RELAY2, relay2ShouldBeOn ? 1 : 0);

  /**** 5 – Drive motors *******************************************/
  ledcWrite(ENA, pwmA);
  ledcWrite(ENB, pwmB);

  /**** 6 – Update globals for dashboard ***************************/
  gTempC = tempC;
  gIac   = I_AC;
  gIrBlk = obstacle;      // TRUE = something blocking

  /**** 7 – Serial debug *******************************************/
  Serial.printf("T:%.1f°C  IR:%s  PWM_A:%3d PWM_B:%3d  I_AC:%.2fA  R1:%s  R2:%s\n",
                tempC,
                obstacle ? "BLOCK" : "CLEAR",
                pwmA, pwmB,
                I_AC,
                relay1ShouldBeOn ? "ON" : "OFF",
                relay2ShouldBeOn ? "ON" : "OFF");

  /**** 8 – Run Blynk timers ***************************************/
  Blynk.run();
  timer.run();
  delay(200);   // ~5 Hz loop
}

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm(0x40);

// ===== PCA9685 / Servo config =====
const int SERVO_FREQ = 50;
const int SERVOMIN   = 110;   // adjust if needed
const int SERVOMAX   = 500;   // adjust if needed

const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ===== Breath tuning =====
// Deeper + quieter range (reduce strain/slack vs full 0..180)
const int BREATH_MIN_ANG = 0;     // raise if slack
const int BREATH_MAX_ANG = 180;    // lower if strain

// Deeper feel (linger at ends). Reduce if motion becomes snappy/noisy.
const float DEPTH_SHAPE = 1.6f;

// Make all start SAME, then spread phases over this time
const unsigned long PHASE_RAMP_MS = 3000; // 3s

// Update cadence (slower updates can reduce chatter/noise)
const unsigned long UPDATE_EVERY_MS = 30;

static constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

// ===== Servo state =====
struct ServoState {
  uint8_t ch;
  unsigned long t0;
  unsigned long lastMs;
  float targetPhaseRad;     // final phase offset
  unsigned long periodMs;   // full cycle duration
  float downFrac;           // fraction of cycle spent going DOWN (toward BREATH_MIN_ANG)
};

// ===== Helpers =====
uint16_t angleToTick(int angle) {
  angle = constrain(angle, 0, 180);
  return (uint16_t)map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void moveServo(uint8_t ch, int angle) {
  pwm.setPWM(ch, 0, angleToTick(angle));
}

float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

int lerpAngle(int a, int b, float t) {
  return (int)(a + (b - a) * t + 0.5f);
}

// Linger near inhale/exhale (deeper, more organic)
float deepen(float v, float shape) {
  v = clamp01(v);
  if (shape <= 1.01f) return v;

  float x = (v - 0.5f) * 2.0f;     // -1..1
  float s = (x >= 0) ? 1.0f : -1.0f;
  x = fabsf(x);                    // 0..1
  x = powf(x, shape);              // stronger => more end-dwell
  float y = s * x;                 // -1..1
  return (y * 0.5f) + 0.5f;        // 0..1
}

// ===== 3 servos =====
// All start from same angle, then phase offsets ramp to these targets.
// Channel 1 goes DOWN faster (smaller downFrac).
ServoState servos[3] = {
  {0, 0, 0,   0.0f * DEG2RAD, 3500, 0.50f}, // ch0 normal (symmetric)
  {1, 0, 0, 120.0f * DEG2RAD, 14000, 0.35f}, // ch1: faster DOWN, slower UP
  {2, 0, 0, 240.0f * DEG2RAD, 14000, 0.50f}, // ch2 normal (symmetric)
};

// Compute breath angle with a *current* phase that ramps 0 -> targetPhaseRad
// Asymmetric timing: UP takes (1-downFrac) of the cycle, DOWN takes downFrac.
int patternBreath(unsigned long now, const ServoState &s) {
  // ramp factor 0..1 (phase spread-in)
  float r = clamp01((float)(now - s.t0) / (float)PHASE_RAMP_MS);
  float phaseNow = s.targetPhaseRad * r;

  // Start wave at EXHALE minimum: sin(-PI/2) = -1  => v=0 => BREATH_MIN_ANG
  const float START_SHIFT = -PI * 0.5f;

  unsigned long p = s.periodMs;
  float t = (float)((now - s.t0) % p) / (float)p; // 0..1

  // Guard against extremes
  float downFrac = clamp01(s.downFrac);
  if (downFrac < 0.05f) downFrac = 0.05f;
  if (downFrac > 0.95f) downFrac = 0.95f;

  // We start at MIN and go UP first, then DOWN back to MIN.
  float upFrac = 1.0f - downFrac;

  float v;
  if (t < upFrac) {
    // UP portion: map 0..upFrac -> 0..1
    float u = t / upFrac;                      // 0..1
    float a = START_SHIFT + (PI * u) + phaseNow; // -pi/2 .. +pi/2
    v = (sinf(a) + 1.0f) * 0.5f;               // 0..1
  } else {
    // DOWN portion: map upFrac..1 -> 0..1
    float u = (t - upFrac) / downFrac;         // 0..1
    float a = (PI * 0.5f) - (PI * u) + phaseNow; // +pi/2 .. -pi/2
    v = (sinf(a) + 1.0f) * 0.5f;               // 0..1
  }

  v = deepen(v, DEPTH_SHAPE);
  return lerpAngle(BREATH_MIN_ANG, BREATH_MAX_ANG, v);
}

// ===== Per-servo updater =====
void updateServo(ServoState &s, unsigned long now) {
  if (now - s.lastMs < UPDATE_EVERY_MS) return;
  s.lastMs = now;

  int ang = patternBreath(now, s);
  moveServo(s.ch, ang);
}

// ===== Arduino entry points =====
void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  unsigned long now = millis();
  for (auto &s : servos) {
    s.t0 = now;
    s.lastMs = 0;
  }

  // Force all to SAME starting angle immediately
  int startAng = BREATH_MIN_ANG;
  for (auto &s : servos) moveServo(s.ch, startAng);
}

void loop() {
  unsigned long now = millis();
  for (auto &s : servos) updateServo(s, now);
}

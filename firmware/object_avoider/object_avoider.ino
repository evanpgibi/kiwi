// ============================================================
// 2-Wheel IR Obstacle Avoidance Robot
// Hardware: Arduino Nano + L298N + 3x IR Sensors
// ============================================================

// --- Motor Driver Pins (L298N) ---
// Left Motor
const int ENA = 5;   // PWM speed control - Left
const int IN1 = 6;   // Left motor direction
const int IN2 = 7;

// Right Motor
const int ENB = 10;  // PWM speed control - Right
const int IN3 = 8;   // Right motor direction
const int IN4 = 9;

// --- IR Sensor Pins ---
// IR sensors output LOW when obstacle detected (adjust if yours are inverted)
const int IR_LEFT   = A0;
const int IR_CENTER = A1;
const int IR_RIGHT  = A2;

// --- Tuning Parameters ---
const int BASE_SPEED      = 150;  // Forward speed (0-255)
const int TURN_SPEED      = 140;  // Turn speed
const int TURN_45_MS      = 300;  // ~45 degree turn duration (tune this)
const int TURN_360_MS     = 600; // ~360 degree spin duration (tune this)
const int BACKUP_MS       = 400;  // Reverse duration before 360 spin

// IR sensor logic: most modules output LOW when obstacle detected
// Set this to HIGH if your sensors are inverted
const int OBSTACLE = LOW;

// ============================================================
// SETUP
// ============================================================
void setup() {
  // Motor pins as output
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // IR sensor pins as input
  pinMode(IR_LEFT,   INPUT);
  pinMode(IR_CENTER, INPUT);
  pinMode(IR_RIGHT,  INPUT);

  Serial.begin(9600);
  Serial.println("Robot initialized. Moving forward...");

  stopMotors();
  delay(1000); // Small pause before starting
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  bool leftDetected   = (digitalRead(IR_LEFT)   == OBSTACLE);
  bool centerDetected = (digitalRead(IR_CENTER) == OBSTACLE);
  bool rightDetected  = (digitalRead(IR_RIGHT)  == OBSTACLE);

  if (centerDetected) {
    // Center obstacle: back up then do a 360
    Serial.println("CENTER detected -> Backup + 360 spin");
    stopMotors();
    delay(100);
    moveBackward();
    delay(BACKUP_MS);
    stopMotors();
    delay(100);
    spinRight(TURN_360_MS);  // Full 360 spin
    stopMotors();
    delay(100);

  } else if (leftDetected) {
    // Left obstacle: turn right ~45 degrees
    Serial.println("LEFT detected -> Turn Right ~45deg");
    stopMotors();
    delay(100);
    turnRight(TURN_45_MS);
    stopMotors();
    delay(100);

  } else if (rightDetected) {
    // Right obstacle: turn left ~45 degrees
    Serial.println("RIGHT detected -> Turn Left ~45deg");
    stopMotors();
    delay(100);
    turnLeft(TURN_45_MS);
    stopMotors();
    delay(100);

  } else {
    // All clear: move forward
    moveForward();
  }
}

// ============================================================
// MOTOR CONTROL FUNCTIONS
// ============================================================

void moveForward() {
  // Left motor forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, BASE_SPEED);

  // Right motor forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, BASE_SPEED);
}

void moveBackward() {
  // Left motor backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, BASE_SPEED);

  // Right motor backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, BASE_SPEED);
}

// Turn right: left motor forward, right motor backward (pivot)
void turnRight(int duration) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, TURN_SPEED);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, TURN_SPEED);

  delay(duration);
}

// Turn left: right motor forward, left motor backward (pivot)
void turnLeft(int duration) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, TURN_SPEED);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, TURN_SPEED);

  delay(duration);
}

// Full spin right (360)
void spinRight(int duration) {
  turnRight(duration);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 0);
}

#define FLOW_COLD_pin 32
#define FLOW_HOT_pin 33

uint32_t millis1 = 0;

volatile uint32_t flow_cold_us = 0;
volatile uint32_t flow_hot_us = 0;

volatile uint32_t timer_flow_cold_us = 0;
volatile uint32_t timer_flow_hot_us = 0;

volatile uint32_t timeout_flow_cold_us = 0;
volatile uint32_t timeout_flow_hot_us = 0;

hw_timer_t *timer_10us = NULL;

static portMUX_TYPE mutex_int1 = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE mutex_int2 = portMUX_INITIALIZER_UNLOCKED;

void ARDUINO_ISR_ATTR timer_10us_ISR() {
  if (timeout_flow_cold_us > 0) {
    timeout_flow_cold_us--;
    timer_flow_cold_us++;
  } else {
    flow_cold_us = 0;
    timer_flow_cold_us = 0;
  }

  if (timeout_flow_hot_us > 0) {
    timeout_flow_hot_us--;
    timer_flow_hot_us++;
  } else {
    flow_hot_us = 0;
    timer_flow_hot_us = 0;
  }
}

void IRAM_ATTR flow_cold_ext_ISR() {
  if (digitalRead(FLOW_COLD_pin) == LOW) {
    flow_cold_us = timer_flow_cold_us;
    timer_flow_cold_us = 0;
    timeout_flow_cold_us = 100000;
  }
}

void IRAM_ATTR flow_hot_ext_ISR() {
  if (digitalRead(FLOW_HOT_pin) == LOW) {
    flow_hot_us = timer_flow_hot_us;
    timer_flow_hot_us = 0;
    timeout_flow_hot_us = 100000;
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(FLOW_COLD_pin, INPUT_PULLUP);
  pinMode(FLOW_HOT_pin, INPUT_PULLUP);

  Serial.begin(115200);  // Initialize the serial communication:
  delay(1000);

  Serial.println("Start");

  // Note: CHANGE mode resulted in better stability
  attachInterrupt(FLOW_COLD_pin, flow_cold_ext_ISR, CHANGE);  // CHANGE FALLING RISING
  attachInterrupt(FLOW_HOT_pin, flow_hot_ext_ISR, CHANGE);    // CHANGE FALLING RISING

  timer_10us = timerBegin(100000);                    // <<< Set timer frequency Mhz
  timerAttachInterrupt(timer_10us, &timer_10us_ISR);  // <<< Attach Timer0_ISR function to our timer.
  // Set alarm to call onTimer function(value in microseconds x10).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(timer_10us, 1, true, 0);  // <<< 10us
}

void loop() {
  // put your main code here, to run repeatedly:
  if ((millis() - millis1) >= 200) {
    millis1 = millis();
    float freq = 0;
    float flow_cold = 0;
    float rate_flow_cold = 0;
    float flow_hot = 0;
    float rate_flow_hot = 0;
    float flow_total = 0;

    if (flow_cold_us != 0) {
      freq = 100000;
      freq /= flow_cold_us;
      flow_cold = freq;
      flow_cold /= 7.5;
    }

    freq = 0;

    if (flow_hot_us != 0) {
      freq = 100000;
      freq /= flow_hot_us;
      flow_hot = freq;
      flow_hot /= 7.5;
    }

    flow_total = flow_cold;
    flow_total += flow_hot;

    if (flow_total >= 0.5) {
      if (flow_cold != 0) {
        rate_flow_cold = flow_cold;
        rate_flow_cold /= flow_total;
        rate_flow_cold *= 100;
      }

      if (flow_hot != 0) {
        rate_flow_hot = flow_hot;
        rate_flow_hot /= flow_total;
        rate_flow_hot *= 100;
      }
    }

    Serial.print("Cold: ");
    Serial.print(flow_cold, 5);

    Serial.print(" [");
    Serial.print(rate_flow_cold);

    Serial.print("%] Hot: ");
    Serial.print(flow_hot, 5);

    Serial.print(" [");
    Serial.print(rate_flow_hot);

    Serial.print("%] Total: ");
    Serial.println(flow_total, 5);
  }
}

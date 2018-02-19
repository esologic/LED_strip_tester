#include "FastLED.h"

#define NUM_LEDS 200

#define CLOCK_PIN 9
#define CLK_DATA_PIN 10
#define TIME_DATA_PIN 11

CRGB clk_leds[NUM_LEDS];
CRGB time_leds[NUM_LEDS];

#define number_of_74hc595s 2 // Number of Shift Registers
#define NUM_REG_PINS number_of_74hc595s * 8
boolean registers[NUM_REG_PINS];

#define NUM_CHARS 36

#define SEG_0 0
#define SEG_1 1
#define SEG_2 2
#define SEG_3 3
#define SEG_4 4
#define SEG_5 5
#define SEG_6 6
#define SEG_7 7
#define SEG_8 8
#define SEG_9 9
#define SEG_A 10
#define SEG_B 11
#define SEG_C 12
#define SEG_D 13
#define SEG_E 14
#define SEG_F 15
#define SEG_G 16
#define SEG_H 17
#define SEG_I 18
#define SEG_J 19
#define SEG_K 20
#define SEG_L 21
#define SEG_M 22
#define SEG_N 23
#define SEG_O 24
#define SEG_P 25
#define SEG_Q 26
#define SEG_R 27
#define SEG_S 29
#define SEG_T 30
#define SEG_U 31
#define SEG_V 32
#define SEG_W 33
#define SEG_X 34
#define SEG_Y 35
#define SEG_Z 36

int num_to_seg[10] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, SEG_8, SEG_9};

#define CATH_A 13
#define CATH_B 15
#define CATH_C 12
#define CATH_D 2
#define CATH_E 4
#define CATH_F 10
#define CATH_G 14
#define CATH_DP 6

#define ANOD_1 0
#define ANOD_2 1
#define ANOD_4 3
#define ANOD_6 5
#define ANOD_8 7

#define NUM_SEGS 4
int seg_to_update = 0;
unsigned long previous_update_time = 0;
int refresh_rate = 0;
int cathodes[NUM_SEGS] = {ANOD_1, ANOD_2, ANOD_6, ANOD_8}; 
int seg_values[NUM_SEGS] = {0, 0, 0, 0};
int anode_configs[4][NUM_SEGS] = { {HIGH, LOW, LOW, LOW}, {LOW, HIGH, LOW, LOW}, {LOW, LOW, HIGH, LOW}, {LOW, LOW, LOW, HIGH}};

int SREG_SER_PIN = 2; // on the arduino digital pins
int SREG_OE_PIN = 3; // on the arduino digital pins
int SREG_RCLK_PIN = 4; // on the arduino digital pins
int SREG_SRCLK_PIN = 5; // on the arduino digital pins
int SREG_SRCLR_PIN = 6; // on the arduino digital pins

#define NUM_BUTTONS 2

int up_button_pin = 7;
int down_button_pin = 8;

int buttons[NUM_BUTTONS] = {up_button_pin, down_button_pin};
int button_states[NUM_BUTTONS]; // the current reading from the input pin
int last_button_states[NUM_BUTTONS] = {LOW, LOW};   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long last_debounce_times[NUM_BUTTONS] = { 0 };  // the last time the output pin was toggled
unsigned long debounce_delay = 20;    // the debounce time; increase if the output flickers

unsigned long previous_blink_time = 0;

int count = 0;


unsigned long previous_change = 0;
int color_num = 0; 
#define NUM_COLORS 3
CRGB colors[NUM_COLORS] = {CRGB::Red, CRGB::Green, CRGB::Blue}; 

void setup() {

  Serial.begin(9600);

  FastLED.addLeds<APA102, CLK_DATA_PIN, CLOCK_PIN, BGR>(clk_leds, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, TIME_DATA_PIN>(time_leds, NUM_LEDS);
  
  pinMode(SREG_SER_PIN, OUTPUT);
  pinMode(SREG_OE_PIN, OUTPUT);
  pinMode(SREG_RCLK_PIN, OUTPUT);
  pinMode(SREG_SRCLK_PIN, OUTPUT);
  pinMode(SREG_SRCLR_PIN, OUTPUT);

  pinMode(up_button_pin, INPUT);
  pinMode(down_button_pin, INPUT);

  digitalWrite(SREG_OE_PIN, LOW);
  digitalWrite(SREG_SRCLR_PIN, HIGH);

  set_write(ANOD_1, LOW);
  set_write(ANOD_2, LOW);
  set_write(ANOD_4, LOW);
  set_write(ANOD_6, LOW);
  set_write(ANOD_8, LOW);

}

void loop() {
  
  process_buttons();
  
  set_number(count);

  update_display();

  unsigned long current_time = millis();

  if (current_time - previous_change > 1000) {
    color_num++;
    if (color_num >= NUM_COLORS) {
      color_num = 0;
    }
    previous_change = current_time;
  }

  for (int i = 0; i < count; i++) {
    clk_leds[i] = colors[color_num];
    time_leds[i] = colors[color_num];
  }
  FastLED.show();

  for (int i = count; i < NUM_LEDS; i++) {
    clk_leds[i] = CRGB::Black;
    time_leds[i] = CRGB::Black;
  }
  FastLED.show();
 
  

}

void process_buttons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {

    int reading = digitalRead(buttons[i]);
  
    if (reading != last_button_states[i]) {
      last_debounce_times[i] = millis();
    }
    
    if ((millis() - last_debounce_times[i]) > debounce_delay) {
      if (reading != button_states[i]) {
        button_states[i] = reading;
        if (button_states[i] == HIGH) {

          if (buttons[i] == up_button_pin) {
            if (count !=  NUM_LEDS) {
              count++;
            }
          } else {
            if (count !=  0) {
              count--;
            }
          }
            
        }
      }
    }
  
    last_button_states[i] = reading;
  }
}

void update_display() {
  
  if (millis() - previous_update_time > refresh_rate){
    
    set_cathodes(seg_values[seg_to_update]); 
    for (int j = 0; j < NUM_SEGS; j++) {
      set_write(cathodes[j], anode_configs[seg_to_update][j]);
    }
  
    seg_to_update = (seg_to_update + 1) % 4;

    previous_update_time = millis();
    
  }

}

void set_number(int number) {
  seg_values[3] = num_to_seg[(number % 10)];
  seg_values[2] = num_to_seg[((number / 10) % 10)];
  seg_values[1] = num_to_seg[((number / 100) % 10)];
  seg_values[0] = num_to_seg[(number / 1000)];
}

void set_cathodes(int seg_char) {

  set_register_pin(CATH_A, HIGH);
  set_register_pin(CATH_B, HIGH);
  set_register_pin(CATH_C, HIGH);
  set_register_pin(CATH_D, HIGH);
  set_register_pin(CATH_E, HIGH);
  set_register_pin(CATH_F, HIGH);
  set_register_pin(CATH_G, HIGH);
  set_register_pin(CATH_DP, HIGH);

  switch (seg_char) {

    case SEG_0:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_B, LOW);
      break;

    case SEG_1:
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_2:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_3:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_4:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_5:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_6:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_G, LOW);
      break;

    case SEG_7:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_8:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_G, LOW);
      break;

    case SEG_9:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_A:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_B:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_C:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_D:
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_E:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_F:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      break;

    case SEG_G:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_H:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_I:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      break;

    case SEG_J:
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_E, LOW);
      break;

    case SEG_K:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_G, LOW);
      break;

    case SEG_L:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_M:
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_A, LOW);
      break;

    case SEG_N:
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_O:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_B, LOW);
      break;

    case SEG_P:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_E, LOW);
      break;

    case SEG_Q:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_R:
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      break;

    case SEG_S:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_T:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_U:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_B, LOW);
      break;

    case SEG_V:
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_W:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_X:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_C, LOW);
      break;

    case SEG_Y:
      set_register_pin(CATH_F, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_C, LOW);
      set_register_pin(CATH_D, LOW);
      break;

    case SEG_Z:
      set_register_pin(CATH_A, LOW);
      set_register_pin(CATH_B, LOW);
      set_register_pin(CATH_G, LOW);
      set_register_pin(CATH_E, LOW);
      set_register_pin(CATH_D, LOW);
      break;
  }

  write_registers();

}

void set_write(int index, int value) {
  set_register_pin(index, value);
  write_registers();
}

void clear_registers() {
  for (int i = NUM_REG_PINS - 1; i >= 0; i--) {
    registers[i] = LOW;
  }
}

void write_registers() {

  digitalWrite(SREG_RCLK_PIN, LOW);
  digitalWrite(SREG_OE_PIN, HIGH);

  for (int i = NUM_REG_PINS - 1; i >= 0; i--) {
    digitalWrite(SREG_SRCLK_PIN, LOW);
    digitalWrite(SREG_SER_PIN, registers[i]);
    digitalWrite(SREG_SRCLK_PIN, HIGH);
  }
  digitalWrite(SREG_RCLK_PIN, HIGH);
  digitalWrite(SREG_OE_PIN, LOW);
}

void set_register_pin(int index, int value) {
  registers[index] = value;
}

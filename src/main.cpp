#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ultrasonic.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define HALF_X SCREEN_WIDTH / 2
#define HALF_Y SCREEN_HEIGHT / 2
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SERVO_PIN 9
#define BUZZER_PIN 8
#define ULTRASONIC_TRIGGER 6
#define ULTRASONIC_ECHO 5
#define MIN_DISTANCE 64
#define LASER_PIN 7
#define CIRCLE_RADIUS 8

Servo servo;
Ultrasonic ultrasonic(ULTRASONIC_TRIGGER,ULTRASONIC_ECHO);
uint8_t servo_pos = 0;


enum ESTADO_TORRETA_t {
  REPOSO,
  RAPIDO,
  DISPARO
};

enum COLOR_LED_t {
  Rojo,
  Verde,
  Azul,
  Amarillo
};


ESTADO_TORRETA_t estado_torreta = REPOSO;
COLOR_LED_t estado_led = Verde;
int8_t scala = 1;
uint16_t distancia = 0;
uint8_t distancia_porcentaje = 0;

uint16_t last_tone_ms = 0;
uint16_t last_laser_ms = 0;
uint16_t buzzer_trigger_ms = 100; // baseline for the buzzer beeps
uint16_t laser_trigger_ms = 50;
bool on_sweep_finished = false;
bool laser_state = 0;
bool buzzer_off = false;

bool rango(int valor, int minimo, int maximo){
  return (valor < maximo and valor > minimo);
}

void move(int8_t increment){
  servo_pos += (increment * scala);
  if (servo_pos >= 180){scala = -1;}
  if(servo_pos <= 0){scala = 1;}
}

void torreta_tick(){
  distancia = ultrasonic.read();
  //distancia_porcentaje = round(140.0f * (static_cast<float>(distancia) / 100.0f));
  distancia_porcentaje = map(distancia, 0,MIN_DISTANCE,0,100);
  if(rango(distancia_porcentaje,51,100)){estado_torreta = REPOSO;}
  if(rango(distancia_porcentaje,6,50)){estado_torreta = RAPIDO;}
  if(rango(distancia_porcentaje,0,16)){estado_torreta = DISPARO;}

  Serial.println(distancia_porcentaje);
  servo.write(servo_pos);
}
/*
void laser_tick(){
  if (buzzer_off){digitalWrite(LASER_PIN,0); return;}
  if (millis() - last_laser_ms >= laser_trigger_ms){
    last_laser_ms = millis();
    laser_state = !laser_state;
    digitalWrite(LASER_PIN, laser_state);
  }

}
  */

void buzzer_tick(){
    if (buzzer_off){
      digitalWrite(LASER_PIN,0);


      return;
    }
  digitalWrite(LASER_PIN,laser_state);

  uint8_t porcentaje_inverso = map(distancia_porcentaje,0,100,100,0);
  if(porcentaje_inverso > 100){porcentaje_inverso = 100;}
  buzzer_trigger_ms = distancia_porcentaje +1 * 10; // to make 100% 1khz;
  if (millis() - last_tone_ms >= buzzer_trigger_ms){
    
    tone(BUZZER_PIN,porcentaje_inverso * 10,buzzer_trigger_ms / 2);
    last_tone_ms = millis();
    laser_state = !laser_state;


  }
}

uint16_t last_render_x, last_render_y;

void update_latest_state(){

  switch (estado_torreta)
  {
  case REPOSO:
  move(1);
    buzzer_off = true;
    break;
  case RAPIDO:
    laser_trigger_ms = 100;
    buzzer_off = false;
    move(3);
    break;
  case DISPARO:
    laser_trigger_ms = 10;
    buzzer_off = false;

    break;
  
  default:
    break;
  }

  if((distancia >= MIN_DISTANCE)){return;}
  last_render_y = map(distancia,0,MIN_DISTANCE,SCREEN_HEIGHT,0);
  last_render_x = map(servo_pos,0,180,0,SCREEN_WIDTH);

}

void render(){
  display.clearDisplay();

  uint8_t num_circles = 4;
  uint8_t max_radius = SCREEN_WIDTH / 2; 
  uint8_t step = max_radius / num_circles;
  uint8_t y_pos = SCREEN_HEIGHT;

  for(uint8_t i = 1; i <= num_circles; i++){
    uint8_t radius = i * step;
    display.drawCircle(HALF_X, y_pos, radius, WHITE);
  }
  float rad = servo_pos * (PI / 180.0);
  int16_t x_line = HALF_X + (max_radius * cos(rad));
  int16_t y_line = y_pos - (max_radius * sin(rad));
  
  if((distancia <= MIN_DISTANCE)){last_render_x = x_line;}

  display.drawLine(HALF_X, y_pos, x_line, y_line, WHITE);
  display.fillCircle(last_render_x, last_render_y, CIRCLE_RADIUS, WHITE);
  display.display();
}

void setup() {
  Serial.begin(9600);
 if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));}
    servo.attach(SERVO_PIN);

Serial.print("HOLA");
pinMode(LASER_PIN,OUTPUT);
}

void loop(){
  torreta_tick();
  update_latest_state();
  buzzer_tick();
  //laser_tick();
  render();
  delay(50);
  //Serial.println(laser_state);
}

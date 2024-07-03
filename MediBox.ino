
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels 
#define SCREEN_HEIGHT 64 // OLED display height, in pixels 
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; ex3D for 128x64, 0x3C for 128x32

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35

#define DHTPIN 12

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0

//Global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

int Secs;
int Mins;
int Hrs;
int Days;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0,1,2};
int alarm_minutes[] = {1,10,20};
bool alarm_triggered[] = {false, false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G ,A, B,C_H};

int current_mode = 0;
int max_modes = 6;
String modes[] = {"1 - Set    Time","2 - Set    Time Zone", "3 - Set    Alarm 1", "4 - Set    Alarm 2","5 - Set    Alarm 3", "6- Disable Alarms"};
int current_time_zone = 0;
int max_time_zones = 6;
String time_zones[] = {"1- UTC-05:00", "2- UTC-03:00", "3- UTC+00:00", "4- UTC+05:30", "5- UTC+09:00", "6- UTC+11:00"};


//Declare objects
Adafruit_SSD1306 display (SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor; 

void setup() {

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_UP, INPUT);

  Serial.begin(9600);
  // put your setup code here, to run once:
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen -- 
  // the library initializes this with an Adafruit splash screen. 
  display.display();
  delay(2000); // Pause for 2 seconds
  
  // Clear the buffer 
  display.clearDisplay();
 

  print_line("Welcome to Medibox!", 10, 20, 2);
  display.clearDisplay();

  dhtSensor.setup(DHTPIN,DHTesp::DHT22);
  
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0,0,2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0,0,2);
  display.clearDisplay();

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  startup_time();  
}  
  
  void loop() {
    
    if (digitalRead(PB_OK)==LOW){
      delay(200);
      go_to_menu();
    }

     Secs = seconds;
     Mins = minutes;
     Hrs = hours;
     Days = days;

    update_time_with_check_alarm_and_check_temp();
     
}

void print_line(String text, int column, int row, int text_size){
  
  display.setTextSize(text_size);               
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(column,row);              //(column, row)
  display.println(text); 
  
  display.display();
  
}

void print_line_black(String text, int column, int row, int text_size){
  
  display.setTextSize(text_size);               
  display.setTextColor(BLACK);  // Draw black text
  display.setCursor(column,row);              //(column, row)
  display.println(text); 
  
  display.display();
}


// function to display the current time DD:HH:MM:SS 
void print_time_now(void) {
  if (Days != days){
      print_line_black(String(Days), 0, 0, 2); 
      print_line_black(":", 20, 0, 2);
      print_line(String(days), 0, 0, 2); 
      print_line(":", 20, 0, 2);
  }

  if (Hrs != hours){
      print_line_black(String (Hrs), 30,0,2);
      print_line_black(":", 50, 0, 2);
      print_line(String (hours), 30,0,2);
      print_line(":", 50, 0, 2);
  }

  if (Mins != minutes){
      print_line_black(String(Mins), 60, 0, 2);
      print_line_black(":", 80, 0, 2);
      print_line(String(minutes), 60, 0, 2);
      print_line(":", 80, 0, 2);
  }
  
  if (Secs != seconds){
      
      print_line_black(String(Secs), 90, 0, 2);
      print_line(String(seconds), 90, 0, 2);
      
  }

}

// function to automatically update the current time 
void update_time(void) {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3,"%H",&timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3,"%M",&timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3,"%S",&timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d",&timeinfo);
  days = atoi(timeDay);
}

// ring an alarm
void ring_alarm() {
  // Show message on display
  display.clearDisplay();
  print_line("Medicine Time!", 0, 0, 2);
  
  bool break_happened = false;
  
  // ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }  
        tone (BUZZER, notes[i]);
        delay(500);
        noTone (BUZZER);
        delay(2);
        digitalWrite(LED_1, HIGH);   //Make the LED blink when the alarm is ringing
    }    
  delay(200);
  digitalWrite(LED_1, LOW);
  }

  display.clearDisplay();
  startup_time();

}

// function to automatically update the current time while checking for alarms
void update_time_with_check_alarm_and_check_temp() {
  
  update_time();             // update time
  print_time_now();
  check_temp();
  delay(200);
  
  // check for alarms
  if (alarm_enabled) {
  // iterating through all alarms
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i]==hours && alarm_minutes[i]==minutes) {
        ring_alarm(); // call the ringing function
        alarm_triggered[i] = true;
      }
    }
  }
  
}

// function to wait for button press in the menu
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }

    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }

    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }  
    
    else if (digitalRead(PB_OK) == LOW ) {
      delay(200);
      return PB_OK;
    }

    update_time();
  }
}


// function to navigate through the menu 
void go_to_menu() {                   
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    delay(1000);
  
    int pressed = wait_for_button_press();
  
    if (pressed == PB_UP) {
      current_mode += 1;
      current_mode %= max_modes; 
      delay(200);
    } 
      
    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
      current_mode = max_modes - 1;
      }
    }
  
    else if (pressed == PB_OK) {
      Serial.println(current_mode);
      delay(200);
      run_mode (current_mode);
    }  

    else if (pressed == PB_CANCEL) {
      delay(200);
      display.clearDisplay();
      startup_time();
      break;
    }  
  }
}


 
void set_time() {         //Function to set time
  int temp_hour = hours;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String (temp_hour), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }  
    else if (pressed==PB_DOWN) {
      delay(200);
      temp_hour -= 1;
    
      if (temp_hour < 0) {
        temp_hour = 23;
    }
  }
    
    else if (pressed== PB_OK) {
      delay(200);
      hours = temp_hour;
      break;
    }
    
    else if (pressed==PB_CANCEL) {
      delay(200);
      break;
    }
  }  

 int temp_minute = minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String (temp_minute), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }  
    else if (pressed==PB_DOWN) {
      delay(200);
      temp_minute -= 1;
    
      if (temp_minute < 0) {
        temp_minute = 59;
    }
  }
    
    else if (pressed== PB_OK) {
      delay(200);
      minutes = temp_minute;
      break;
    }
    
    else if (pressed==PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  delay(1000);
}  


void set_alarm(int alarm) {             //Function to set an alarm
  int temp_hour =alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String (temp_hour), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }  
    else if (pressed==PB_DOWN) {
      delay(200);
      temp_hour -= 1;
    
      if (temp_hour < 0) {
        temp_hour = 23;
    }
  }
    
    else if (pressed== PB_OK) {
      delay(200);
     alarm_triggered[alarm]= false;
     alarm_hours[alarm] = temp_hour;
      break;
    }
    
    else if (pressed==PB_CANCEL) {
      delay(200);
      break;
    }
  }  

 int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String (temp_minute), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }  
    else if (pressed==PB_DOWN) {
      delay(200);
      temp_minute -= 1;
    
      if (temp_minute < 0) {
        temp_minute = 59;
    }
  }
    
    else if (pressed== PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }
    
    else if (pressed==PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}  

void run_mode(int mode) {       //FFunction to select the mode that the medibox needs to be run in
  if (mode == 0) {
    set_time();
  }
  else if (mode == 1){
    set_time_zone();
  }  
  else if (mode ==2 || mode == 3 || mode == 4 ){
    set_alarm(mode-2);
  }
  else if (mode == 5 ){
    alarm_enabled = false;
  }
}

void check_temp(){    //Function to check if the humidity and temperature values are within the healthy range
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32){
    //display.clearDisplay();
    display.fillRect(0, 40, 100, 8, BLACK);
    print_line("HIGH TEMPERATURE",0,40,1);
    delay(2000);

  }
  else if (data.temperature < 26){
    //display.clearDisplay();
    display.fillRect(0, 40, 100, 8, BLACK);
    print_line("LOW TEMPERATURE",0,40,1);
    delay(2000);
  }

  if (data.humidity > 80){
    //display.clearDisplay();
    display.fillRect(0, 55, 90, 8, BLACK);
    print_line("HIGH HUMIDITY",0,55,1);
    delay(2000);
  }
  else if (data.humidity < 60){
    //display.clearDisplay();
    display.fillRect(0, 55, 90, 8, BLACK);
    print_line("LOW HUMIDITY",0,55,1);
    delay(2000);
  }
}

void set_time_zone(){        //Function to give the UTC offset as an input
  while (digitalRead(PB_CANCEL) == HIGH) {
  display.clearDisplay();
  print_line(time_zones[current_time_zone], 0, 0, 2);
  delay(1000);

  int pressed = wait_for_button_press();

  if (pressed == PB_UP) {
    current_time_zone += 1;
    current_time_zone %= max_time_zones; 
    delay(200);
  } 
    
  else if (pressed == PB_DOWN) {
    delay(200);
    current_time_zone -= 1;
    if (current_time_zone < 0) {
    current_time_zone = max_time_zones - 1;
    }
  }

  else if (pressed == PB_OK) {
  Serial.println(current_time_zone);
  delay(200);
  select_current_time(current_time_zone);   //calling the select_current_time function
  break;
  }   

  else if (pressed == PB_CANCEL) {
    delay(200);
    break;
    go_to_menu();
  }  

}
}

void select_current_time(int time_zone){    //Function to select the calculae the UTC offset

  int offset;

   if (time_zone == 0) {
      offset= -5*60*60;           
  } else if (time_zone == 1) {
      offset= -3*60*60 -1*30*60;
  } else if (time_zone == 2) {
      offset= 0*60*60;       
  } else if (time_zone == 3) {
      offset= 5*60*60 +1*30*60;
  } else if (time_zone == 4) {
      offset= 9*60*60;  
  } else if (time_zone == 5) {
      offset= 11*60*60;  
  } 

  configTime(offset, UTC_OFFSET_DST, NTP_SERVER);

}

void startup_time(){         //Function to start displaying time after after exiting the main screen

  update_time();

  print_line(String(days), 0, 0, 2); 
  print_line(":", 20, 0, 2);
  print_line(String (hours), 30,0,2);
  print_line(":", 50, 0, 2);
  print_line(String(minutes), 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);

}
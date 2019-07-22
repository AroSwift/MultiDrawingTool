// Include low level code for Adafruit color display (by Adafruit)
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

// Color 1.44" TFT output pins
const int cs_pin = 10;
const int reset_pin = 9;
const int dc_pin = 8;

// Initialize the colorful 1.44" TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(cs_pin,  dc_pin, reset_pin);

const int screen_size = 128;

// Input pins
const int pot_pin_1 = A0;
const int pot_pin_2 = A1;
const int pot_pin_3 = A2;
const int mode_btn_pin = 2;
const int color_btn_pin = 5;

// Clock variables
int radius = 60;
int center = tft.width()/2;
int clock_delay_time = 985;
int sec_increment = 56;
int clock_increment = 0;
int clock_quad = 0;

// Continuous drawing variables
int total_space;
int individual_space;
int lines[screen_size][3];
int current_color_index = 0;
const uint16_t possible_colors[4] = { ST7735_BLACK, ST7735_RED, ST7735_GREEN, ST7735_BLUE };

// Mode variables
boolean switched = false;
int current_mode = 0;

void setup() {
  // Initialize the 1.44" tft ST7735S chip with a white background
  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST7735_WHITE);

  // Initialize the default lines position
  for(int i = 0; i < screen_size; i++) {
    for(int j = 0; j < 3; j++) {
      lines[i][j] = -1;
    }
  }

  // Set the pin mode of the mode and color changing buttons
  pinMode(color_btn_pin, INPUT_PULLUP);
  pinMode(mode_btn_pin, INPUT_PULLUP);

  // Attach an interrupt to pin 2 that will call a method to change the mode
  attachInterrupt(digitalPinToInterrupt(mode_btn_pin), switch_mode, LOW);
}

void loop() {
  // Handle when the mode is switched
  handle_switch();

  // When the continuous drawing mode is on
  if(current_mode == 0) {
    // Continuously draw two lines
    continuous_drawer();
  // When the continuous multi drawing mode is on
  } else if(current_mode == 1) {
    // Draw 3 lines continuously
    continuous_multi_drawer();
  } else { // The minute clock mode is on
    // Draw a clock that handles 60 seconds
    minute_clock();
  }
}

// Handle when the mode is switched. When an interrupt
// has been called, clear the screen, update the current
// mode, and clear the most recent mode display.
void handle_switch() {
  // When the mode has been switched
  if(switched) {
    // Clear the screen
    tft.fillScreen(ST7735_WHITE);

    // When the mode can go up
    if(current_mode < 2) {
      // Increment the mode
      current_mode++;
    } else { // The mode needs to cycle
      // Set the mode back to 0
      current_mode = 0;
    }

    // When the mode is any type of continuous drawing
    if(current_mode == 0 || current_mode == 1) {
      // Reset the lines
      for(int i = 0; i < screen_size; i++) {
        for(int j = 0; j < 3; j++) {
          lines[i][j] = -1;
        }
      }
    }

    // When the mode is multi continuous drawing
    if(current_mode == 1) {
      // Get the total dividable space
      total_space = screen_size - (screen_size % 3);
      // Then, get the individual space for each continuous drawing path
      individual_space = total_space / 3;
    // When the current mode is the clock
    } else if(current_mode == 2) {
      // Draw a circle
      tft.drawCircle(tft.width()/2, tft.height()/2, radius, ST7735_BLACK);
      // Reset the clock variables
      clock_quad = 0;
      clock_increment = 0;
    }

    // Only run switched once after an interrupt
    switched = false;
  }
}

// Draw 2 continuous lines across the TFT display such
// that the lines appear to move from left to right.
void continuous_drawer() {
  // Get the mapped potentiometer values for 2 lines
  int pot_value_1 = analogRead(pot_pin_1);
  int pot_value_2 = analogRead(pot_pin_2);
  pot_value_1 = map(pot_value_1, 0, 1023, 0, 128);
  pot_value_2 = map(pot_value_2, 0, 1023, 0, 128);

  // Add the new location to the start of the left screen
  lines[0][0] = pot_value_1;
  lines[0][1] = pot_value_2;

  // Iterate over the line positions
  for(int i = screen_size; i > 0; i--) {
    // Iterate over both line positions
    for(int j = 0; j < 2; j++) {
      // When the position has not been set yet
      if(lines[i][j] == -1) {
        // Move the position to the right
        lines[i][j] = lines[i-1][j];
      } else { // Otherwise, the position is set
        // Erase the previous line
        tft.drawLine(i-1, lines[i-1][j], i, lines[i][j], ST7735_WHITE);
        // Move the line position to the right
        lines[i][j] = lines[i-1][j];
        // Draw the new line which will appear to have moved by one place to the right
        tft.drawLine(i, lines[i][j], i+1, lines[i+1][j], possible_colors[current_color_index]);
      }
    }
  }

  // Handle any updates to the line color
  handle_line_color_change();
}

void continuous_multi_drawer() {
  // Read in the 3 potentiometer values
  int pot_value_1 = analogRead(pot_pin_1);
  int pot_value_2 = analogRead(pot_pin_2);
  int pot_value_3 = analogRead(pot_pin_3);
  // Map the potentiometer values between 3 seperate spaces
  pot_value_1 = map(pot_value_1, 0, 1023, 0, individual_space);
  pot_value_2 = map(pot_value_2, 0, 1023, individual_space, individual_space*2);
  pot_value_3 = map(pot_value_3, 0, 1023, individual_space*2, individual_space*3);

  // Add the new location for each line to the start of the left screen
  lines[0][0] = pot_value_1;
  lines[0][1] = pot_value_2;
  lines[0][2] = pot_value_3;

  // Iterate over the line positions
  for(int i = total_space; i > 0; i--) {
    // Iterate over both line positions
    for(int j = 0; j < 3; j++) {
      // When the position has not been set yet
      if(lines[i][j] == -1) {
        // Move the position to the right
        lines[i][j] = lines[i-1][j];
      } else { // Otherwise, the position is set
        // Erase the previous line
        tft.drawLine(i-1, lines[i-1][j], i, lines[i][j], ST7735_WHITE);
        // Move the line position to the right
        lines[i][j] = lines[i-1][j];
        // Draw the new line which will appear to have moved by one place to the right
        tft.drawLine(i, lines[i][j], i+1, lines[i+1][j], possible_colors[current_color_index]);
      }
    }
  }

  // Handle any updates to the line color
  handle_line_color_change();
}

// Display a analog clock that can display the seconds hand. The second hands
// are handled in quadrants to reduce code. Note, to prevent time dialation, 
// I use .985 seconds because if I did just 1 second, the clock would be slightly
// too fast given the way I designed the clock. Thus, this oddity is warrented.
void minute_clock() {
  // When the clock is in quadrant 1
  if(clock_quad == 0) {
    // Display the second hand, wait .985 seconds, then erase the second hand
    tft.drawLine(center, center, center + clock_increment, 10 + clock_increment, ST7735_BLACK);
    delay(clock_delay_time);
    tft.drawLine(center, center, center + clock_increment, 10 + clock_increment, ST7735_WHITE);
  // When the clock is in quadrant 4
  } else if(clock_quad == 1) {
    // Display the second hand, wait .985 seconds, then erase the second hand
    tft.drawLine(center, center, center + sec_increment - clock_increment, 10 + sec_increment + clock_increment, ST7735_BLACK);
    delay(clock_delay_time);
    tft.drawLine(center, center, center + sec_increment - clock_increment, 10 + sec_increment + clock_increment, ST7735_WHITE);
  // When the clock is in quadrant 3
  } else if(clock_quad == 2) {
    // Display the second hand, wait .985 seconds, then erase the second hand
    tft.drawLine(center, center, center - clock_increment, 10 + sec_increment + sec_increment - clock_increment, ST7735_BLACK);
    delay(clock_delay_time);
    tft.drawLine(center, center, center - clock_increment, 10 + sec_increment + sec_increment - clock_increment, ST7735_WHITE);
  // When the clock is in quadrant 2
  } else if(clock_quad == 3) {
    // Display the second hand, wait .985 seconds, then erase the second hand
    tft.drawLine(center, center, center - sec_increment + clock_increment, 10 + sec_increment - clock_increment, ST7735_BLACK);
    delay(clock_delay_time);
    tft.drawLine(center, center, center - sec_increment + clock_increment, 10 + sec_increment - clock_increment, ST7735_WHITE);
  }

  // When the clock cannot be incremented
  if(clock_increment + 4 > sec_increment) {
    // Reset the clock increment
    clock_increment = 0;

    // When the clock quadrant can be incremented (e.g. minute has not passed)
    if(clock_quad < 3) {
      // Update the clock quadrant
      clock_quad++;
    } else { // Otherwise, a minute has passed
      // Reset the clock quadrant
      clock_quad = 0;
    }
  } else { // When the clock can be incremented
    // Update the second hand position
    clock_increment += 4;
  }
}

// Handle the changing of the continuous line drawing
void handle_line_color_change() {
  // When the color changing button has been pressed
  if(digitalRead(color_btn_pin) == LOW) {

    // And, when the color index doesn't need to loop
    if(current_color_index < 3) {
      // Increment the current color
      current_color_index++;
    } else { // Otherwise, the color index needs to loop
      // Reset the current color index
      current_color_index = 0;
    }

    // Give a small delay to prevent double readings
    delay(200);
  }
}

// This method has been assigned to interrupt such
// that when the mode changing button has been pressed
// (pin 2), this method will be called to change the mode.
void switch_mode() {
  // Indicate that we need to switch modes. Note: this is not done here
  // because issues arrise when "too much" code is in an interrupt method
  switched = true;
}





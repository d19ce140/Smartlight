// HTTP request handlers
 
void handleMinimalUpload() {
  char temp[1500];

  snprintf ( temp, 1500,
   "<!DOCTYPE html>\
    <html>\
      <head>\
        <title>Smart Light∞ Upload</title>\
        <meta charset=\"utf-8\">\
        <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
      </head>\
      <body>\
        <form action=\"/edit\" method=\"post\" enctype=\"multipart/form-data\">\
          <input type=\"file\" name=\"data\">\
          <input type=\"text\" name=\"path\" value=\"/\">\
          <button>Upload</button>\
         </form>\
      </body>\
    </html>"
  );
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send ( 200, "text/html", temp );
}

void handleform() {
//Saperate Colors are sent through javascrip

  String red = server.arg("r");
  String green = server.arg("g");
  String blue = server.arg("b");
  int r = red.toInt();
  int g = green.toInt();
  int b = blue.toInt();
 
  Serial.print("Red:");Serial.println(r);
  Serial.print("Green:");Serial.println(g);
  Serial.print("Blue:");Serial.println(b);
  
  //PWM Correction 8-bit to 10-bit
  r = r * 4; 
  g = g * 4;
  b = b * 4;
 
  //for ULN2003 or Common Cathode RGB Led not needed
  /*
  r = 1024 - r;
  g = 1024 - g;
  b = 1024 - b;
  */
  //ESP supports analogWrite All IOs are PWM
  analogWrite(RedLED,r);
  analogWrite(GreenLED,g);
  analogWrite(BlueLED,b);
 
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated-- Press Back Button");
 
  delay(500);  

}

void handleForm(){
  
  String color = server.arg("color");
  //form?color=%23ff0000
  Serial.println(color);
 
  //See what we have recived
  //We get #RRGGBB in hex string
 
  // Get rid of '#' and convert it to integer, Long as we have three 8-bit i.e. 24-bit values
  long number = (int) strtol( &color[1], NULL, 16);
 
  //Split them up into r, g, b values
  long r = number >> 16;
  long g = (number >> 8) & 0xFF;
  long b = number & 0xFF;
  
  //PWM Correction
  r = r * 4; 
  g = g * 4;
  b = b * 4;
//for ULN2003 or Common Cathode RGB LED not needed
/*
  r = 1024 - r;
  g = 1024 - g;
  b = 1024 - b;
*/
  //ESP supports analogWrite All IOs are PWM
 
  analogWrite(RedLED,r);
  analogWrite(GreenLED,g);
  analogWrite(BlueLED,b);

    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Updated-- Press Back Button");
 
  delay(500);  
    
  }
void setHue(int hue) { // Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
  hue %= 360;                   // hue is an angle between 0 and 359°
  float radH = hue*3.142/180;   // Convert degrees to radians
  float rf, gf, bf;
  
  if(hue>=0 && hue<120){        // Convert from HSI color space to RGB              
    rf = cos(radH*3/4);
    gf = sin(radH*3/4);
    bf = 0;
  } else if(hue>=120 && hue<240){
    radH -= 2.09439;
    gf = cos(radH*3/4);
    bf = sin(radH*3/4);
    rf = 0;
  } else if(hue>=240 && hue<360){
    radH -= 4.188787;
    bf = cos(radH*3/4);
    rf = sin(radH*3/4);
    gf = 0;
  }
  int r = rf*rf*1023;
  int g = gf*gf*1023;
  int b = bf*bf*1023;
  
  analogWrite(RedLED,r);
  analogWrite(GreenLED,g);
  analogWrite(BlueLED,b);
}

void handleSetMainColor(uint8_t * mypayload) {
  // decode rgb data
  uint32_t rgb = (uint32_t) strtol((const char *) &mypayload[1], NULL, 16);
        int r = ((rgb >> 20) & 0x3FF);                     // 10 bits per color, so R: bits 20-29
        int g = ((rgb >> 10) & 0x3FF);                     // G: bits 10-19
        int b = ((rgb >>  0) & 0x3FF);                     // B: bits  0-9

        analogWrite(RedLED, r);
        analogWrite(GreenLED, g);
        analogWrite(BlueLED, b);
}

#include <Wire.h>

#define I2C_ADDRESS 0x42  // ESP32のI2Cスレーブアドレス

volatile uint16_t receivedData = 0;
const int lineSize = 3*32;
uint8_t encoded[lineSize * 4/3 + 3];
String jpegImage = "";

void printImage(){
  Serial.println("#Image");
  Serial.println(jpegImage);
  Serial.println("#End");
  jpegImage = "";
}

void receivePic(int len){
    String str = "";
    while (Wire.available()) {
      byte buff = Wire.read();
      Serial.write(buff);
      str += String( (char) buff);
    }
    if(str != "#EOF"){
      Serial.println();
      jpegImage += str;
    }
    else if(str == "#EOF"){
      Serial.println();
      printImage();
    }

}

void setup() {
  Wire.onReceive(receivePic);
  Wire.begin(I2C_ADDRESS);  // スレーブとして初期化
  Serial.begin(115200);
  delay(2000);
  Serial.println("start");
}

void loop() {
  delay(3000);  // 1秒ごとに表示
}

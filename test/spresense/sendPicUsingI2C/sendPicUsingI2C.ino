#include <Wire.h>
#include <SDHCI.h>
#include <stdio.h>
#include <Camera.h>
#include <LowPower.h>

// #define HEIGHT CAM_IMGSIZE_VGA_H
// #define WIDTH CAM_IMGSIZE_VGA_V

#define HEIGHT CAM_IMGSIZE_QVGA_H
#define WIDTH CAM_IMGSIZE_QVGA_V

// #define USE_HDR
// #define AUTO_AE
// #define AUTO_ISO
#define ESP32_I2C_ADDRESS 0x42  // ESP32のI2Cアドレス
#define AE_WAITING 3000 // 自動露出待ち時間
// #define DEBUG_USB

#ifndef AUTO_ISO
  #define ISO 20000 // 20
  // #define ISO 125000 // 125
  // #define ISO 1000000 // 1000
  // #define ISO 2000000 // 2000
#endif

#ifndef AUTO_AE
  #ifdef USE_HDR// 最大値は 2740(NO HDR)  317(HDR) な模様
    #define AE 100
  #else
    #define AE 2740
  #endif
#endif

const int lineSize = 3*32;
const int sendSize = lineSize * 4/3 + 3;

uint8_t encoded[sendSize];
int sendDelayMilliSecond = 5;

// base64 encoder
const uint8_t base64Table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// spresense側が先に起動しないといけない？クロックを作るために
// インスタンス宣言タイミングだけでいいのか？

void setup() {
  
  Wire.begin();  // I2Cマスターとして初期化
  Serial.begin(115200);
  delay(1000);
  #ifdef DEBUG_USB
    while (!Serial);
  #endif
  initCamera();

  delay(AE_WAITING);

  CamImage img = theCamera.takePicture();
  for(int i = 0 ; i < 10 ; i++){
      img = theCamera.takePicture();
      delay(100);
      int l = img.getImgSize();
      Serial.println(l);
      if(l > 0) break;
    }
  delay(1000);
  while(!img.isAvailable()){
    img = theCamera.takePicture();
    delay(1000);
    int iso = theCamera.getISOSensitivity();
    Serial.print("iso = ");
    Serial.println(iso);

    int32_t ss = theCamera.getAbsoluteExposure();// 露光時間 シャッター速度
    int a = int(1000 /( ss*100/1000 ));
    Serial.print("ss = 1/");
    Serial.print(a); // 100us単位なので
    Serial.print(" , "); // 100us単位なので
    Serial.println(ss); // 100us単位なので

    if(img.isAvailable()) break;


    // iso値を上げていくより、最初はssをいじるべきでは？
    // 今はssを可能な限り下げているけど、その辺りも考慮すべき
    // 暗闇の中でも自動ISOとAUTOシャッタースピードが取得できるのかも調査したい
    // 時間とかは渡すのが面倒なので、最初はAUTOでとって、そこから手動にしていくのがよいかと

    if(!img.isAvailable() && iso < 50000){
        theCamera.setISOSensitivity(50000);
    }
    else if(!img.isAvailable() && iso <= 50000){
        theCamera.setISOSensitivity(80000);
    }
    else if(!img.isAvailable() && iso <= 80000){
        theCamera.setISOSensitivity(125000);
    }
    else if(!img.isAvailable() && iso <= 125000){
        theCamera.setISOSensitivity(200000);
    }
    else if(!img.isAvailable() && iso <= 200000){
        theCamera.setISOSensitivity(320000);
    }
    else if(!img.isAvailable() && iso <= 320000){
        theCamera.setISOSensitivity(640000);
    }
    else if(!img.isAvailable() && iso <= 640000){
        theCamera.setISOSensitivity(800000);
    }
    else if(!img.isAvailable() && iso <= 800000){
        theCamera.setISOSensitivity(1000000);
    }
    else if (!img.isAvailable() && iso <= 1000000){
        theCamera.setISOSensitivity(1600000);
    }
    else if (!img.isAvailable() && iso <= 1600000){
        theCamera.setISOSensitivity(2500000);
    }
    else if (!img.isAvailable() && iso <= 2500000){
        theCamera.setISOSensitivity(3200000);
    }
    else if (!img.isAvailable() && iso <= 3200000){
        theCamera.setISOSensitivity(4000000);
    }
    else if (!img.isAvailable() && iso <= 4000000){
        theCamera.setISOSensitivity(5000000);
    }
    delay(1000);
    for(int i = 0 ; i < 10 ; i++){
      img = theCamera.takePicture();
      delay(100);
      int l = img.getImgSize();
      Serial.println(l);
      if(l > 0) break;
    }
  }
  Serial.println("send pic start");
  sendImageToSerial(img);
  Serial.println("send pic end");
  delay(100);

  LowPower.begin();
  LowPower.deepSleep(60 * 15);
}

void loop() {
   delay(60 * 1000);
}

void base64Encode(uint8_t* encoded, uint8_t* data, int dataLength){
  while(dataLength > 0){
    if(dataLength >= 3){
      encoded[0] = base64Table[data[0] >> 2];
      encoded[1] = base64Table[((data[0]&0x3) << 4) | (data[1] >> 4)];
      encoded[2] = base64Table[((data[1]&0xF) << 2) | (data[2] >> 6)];
      encoded[3] = base64Table[data[2] & 0x3F]; 
      data+=3;
      dataLength -= 3;
    }else if(dataLength == 2){
      encoded[0] = base64Table[data[0] >> 2];
      encoded[1] = base64Table[((data[0]&0x3) << 4) | (data[1] >> 4)];
      encoded[2] = base64Table[(data[1]&0xF) << 2];
      encoded[3] = '=';
      dataLength = 0;
    }else{
      encoded[0] = base64Table[data[0] >> 2];
      encoded[1] = base64Table[(data[0]&0x3) << 4];
      encoded[2] = '=';
      encoded[3] = '=';
      dataLength = 0;
    }
    encoded += 4;
  }
  *encoded = '\0';
}

void printError(enum CamErr err)
{
  Serial.print("Error: ");
  switch (err)
  {
      case CAM_ERR_NO_DEVICE:
    Serial.println("No Device");
    break;
      case CAM_ERR_ILLEGAL_DEVERR:
    Serial.println("Illegal device error");
    break;
      case CAM_ERR_ALREADY_INITIALIZED:
    Serial.println("Already initialized");
    break;
      case CAM_ERR_NOT_INITIALIZED:
    Serial.println("Not initialized");
    break;
      case CAM_ERR_NOT_STILL_INITIALIZED:
    Serial.println("Still picture not initialized");
    break;
      case CAM_ERR_CANT_CREATE_THREAD:
    Serial.println("Failed to create thread");
    break;
      case CAM_ERR_INVALID_PARAM:
    Serial.println("Invalid parameter");
    break;
      case CAM_ERR_NO_MEMORY:
    Serial.println("No memory");
    break;
      case CAM_ERR_USR_INUSED:
    Serial.println("Buffer already in use");
    break;
      case CAM_ERR_NOT_PERMITTED:
    Serial.println("Operation not permitted");
    break;
      default:
    break;
  }
}

void CamCB(CamImage img){ // 自動露光調整のために動かすだけ
  if (img.isAvailable()){
      img.convertPixFormat(CAM_IMAGE_PIX_FMT_JPG);// これ対応してない
      int inputLen = img.getImgSize();
      Serial.println(inputLen);
  }
    else{
      Serial.println("Failed to get video stream image");
  }
}

void initCamera(){
  CamErr err;
  Serial.println("Prepare camera");
  err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS) printError(err);

  
  // カメラストリームを受信したら CamCBを実行する
  // Serial.println("Start streaming");
  // err = theCamera.startStreaming(true, CamCB);
  //   if (err != CAM_ERR_SUCCESS) printError(err);


  // ホワイトバランスの設定
  Serial.println("Set Auto white balance parameter");
  err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_AUTO);
  if (err != CAM_ERR_SUCCESS) printError(err);


  // 静止画フォーマットの設定
  Serial.println("Set still picture format");
  err = theCamera.setStillPictureImageFormat(
    HEIGHT,
    WIDTH,
    CAM_IMAGE_PIX_FMT_JPG);
  if (err != CAM_ERR_SUCCESS) printError(err);
  
  #ifdef AUTO_ISO
    // ISOの設定
    Serial.println("Set ISO Sensitivity AUTO");
    err = theCamera.setAutoISOSensitivity(true);
  #else
    Serial.println("Set ISO Sensitivity = "); // -> 3200 ~ 4000くらいまで
    Serial.println(ISO);
    err = theCamera.setISOSensitivity(ISO);
  #endif
  if (err != CAM_ERR_SUCCESS) printError(err);

  // 露出の設定
  #ifdef AUTO_AE
    Serial.println("Set Auto exposure ");
    err = theCamera.setAutoExposure(true);
  #else
    Serial.print("setAbsoluteExposure = ");
    Serial.println(AE);
    err = theCamera.setAbsoluteExposure(AE);
  #endif
  if (err != CAM_ERR_SUCCESS) printError(err);



  // HDRの設定
  #ifdef USE_HDR
    Serial.println("Set HDR");
    err = theCamera.setHDR(CAM_HDR_MODE_ON);
  #else
    Serial.println("Set HDR OFF");
    err = theCamera.setHDR(CAM_HDR_MODE_OFF);
  #endif
  if (err != CAM_ERR_SUCCESS) printError(err);

}

void sendImageToSerial(CamImage img){
    // Arduino の Wire.write() で一度に送信できるデータの最大バイト数は 32 バイト です。
    int inputLen = img.getImgSize();
    // Serial.print("img.getImgSize() = ");
    // Serial.println(inputLen);
    // Serial.print("to base64-> (size + 2 ) / 3 * 4  = ");
    // Serial.println(int((inputLen + 2 ) / 3 * 4 ));
    uint8_t* p = img.getImgBuff();
    delay(100);

    while(inputLen > 0)
    {
      int len = 0;
      if(inputLen > lineSize) len = lineSize;// データ最終行かどうかのチェック
      else len = inputLen;

      memset(encoded,0,sendSize);
      base64Encode(encoded, p, len); // 1line 96文字
      p += len;
      Serial.println((char*)encoded);
      char a[32] = {0};//必ずしもencodedは128byteじゃない。最後のデータだけ端数が出る
      char b[32] = {0};
      char c[32] = {0};
      char d[32] = {0};

      memcpy(a , encoded , 32);
      // Serial.write(a,32);// printするな。終端文字がない
      Wire.beginTransmission(ESP32_I2C_ADDRESS);
      Wire.write(a);
      Wire.endTransmission();
      // Serial.println();
      delay(sendDelayMilliSecond);

      memcpy(b , encoded + 32 , 32);
      if(strlen(b) == 0) break;
      // Serial.write(b,32);
      Wire.beginTransmission(ESP32_I2C_ADDRESS);
      Wire.write(b);
      Wire.endTransmission();
      // Serial.println();
      delay(sendDelayMilliSecond);

      memcpy(c , encoded + 64 , 32);
      if(strlen(c) == 0) break;
      // Serial.write(c,32);
      Wire.beginTransmission(ESP32_I2C_ADDRESS);
      Wire.write(c);
      Wire.endTransmission();
      // Serial.println();
      delay(sendDelayMilliSecond);

      memcpy(d , encoded + 96 , 32);
      if(strlen(d) == 0) break;
      // Serial.write(d,32);
      Wire.beginTransmission(ESP32_I2C_ADDRESS);
      Wire.write(d);
      Wire.endTransmission();
      // Serial.println();

      delay(sendDelayMilliSecond);

      inputLen = inputLen - len;//次の検索のためにinputLen更新
      if(inputLen <= 0) break;
    }
    Wire.beginTransmission(ESP32_I2C_ADDRESS);
    Wire.write("#EOF");
    Wire.endTransmission();
}

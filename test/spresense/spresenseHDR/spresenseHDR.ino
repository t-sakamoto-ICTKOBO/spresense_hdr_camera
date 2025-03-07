#include <SDHCI.h>
#include <stdio.h>
#include <Camera.h>

// #define BAUDRATE                (1000000)
#define BAUDRATE                (115200)

const int lineSize = 3*32;
uint8_t encoded[lineSize * 4/3 + 3];

// base64 encoder
const uint8_t base64Table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

/**
 * Print error message
 */

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

/**
 * Callback from Camera library when video frame is captured.
 */

void CamCB(CamImage img)
{
  /* Check the img instance is available or not. */
  if (img.isAvailable())
  {
      /* If you want RGB565 data, convert image data format to RGB565 */
      img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);

      /* You can use image data directly by using getImgSize() and getImgBuff().
       * for displaying image to a display, etc. */

      /*
      Serial.print("Image data size = ");
      Serial.print(img.getImgSize(), DEC);
      Serial.print(" , ");

      Serial.print("buff addr = ");
      Serial.print((unsigned long)img.getImgBuff(), HEX);
      Serial.println("");
      */
  }
    else
  {
      Serial.println("Failed to get video stream image");
  }
}

void initCamera(){
  CamErr err;

  /* begin() without parameters means that
   * number of buffers = 1, 30FPS, QVGA, YUV 4:2:2 format */
  Serial.println("Prepare camera");
  err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }
  
  // カメラストリームを受信したら CamCBを実行する
  /*
  Serial.println("Start streaming");
  err = theCamera.startStreaming(true, CamCB);
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }
  */

  // ホワイトバランスの設定
  Serial.println("Set Auto white balance parameter");
  err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_AUTO);
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }

  // 静止画フォーマットの設定
  Serial.println("Set still picture format");
  err = theCamera.setStillPictureImageFormat(
    CAM_IMGSIZE_VGA_H,
    CAM_IMGSIZE_VGA_V,
    CAM_IMAGE_PIX_FMT_JPG);
  if (err != CAM_ERR_SUCCESS)
  {
    printError(err);
  }
  
  // ISOの設定
  Serial.println("Set ISO Sensitivity");
  err = theCamera.setAutoISOSensitivity(true);
  if (err != CAM_ERR_SUCCESS)
  {
    printError(err);
  }
  /*
  // ISO 値を固定する場合は setAutoISOSensitivity(false) に 
  // 使用デバイスがISX012の場合は、最小値はCAM_ISO_SENSITIVITY_25、最大値はCAM_ISO_SENSITIVITY_1600
  // ISX019 HDRカメラのISOはどこまで？調べてみるしかない？
  AUTOにして暗闇でどこまでISOが上がるかを調べてみたら？
  err = theCamera.setISOSensitivity(CAM_ISO_SENSITIVITY_1600);
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }
  */

  // 露出の設定
  Serial.println("Set Auto exposure");
  err = theCamera.setAutoExposure(true);
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }

  /**
   * @brief Set exposure Time
   * @details [en] Set exposure time in 100usec units.  <BR>
   *          [ja] 露光時間(100usec単位)を設定する。
   *
   * @param  exposure_time [en] Exposure time in 100 usec units. ex) 10000 is one second. <BR>
   *                       [ja] 露光時間(100usec単位)。 例) 10000 = 1秒
   * @return [en] Error code defined as #CamErr. <BR>
   *         [ja] #CamErrで定義されているエラーコード。
   */
  // CamErr setAbsoluteExposure(int32_t exposure_time);

 /*
  // 露出を固定する場合は setAutoExposure(false) に 
  int exposure = 2740; // 最大値は 2740(NO HDR)  317(HDR) な模様
  err = theCamera.setAbsoluteExposure(exposure);
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }
*/
  // HDRの設定
  // Serial.println("Set HDR");
  // err = theCamera.setHDR(CAM_HDR_MODE_ON);
  // if (err != CAM_ERR_SUCCESS)
  // {
  //     printError(err);
  // }
  Serial.println("Set HDR OFF");
  err = theCamera.setHDR(CAM_HDR_MODE_OFF);
  if (err != CAM_ERR_SUCCESS)
  {
      printError(err);
  }

}

void sendImageToSerial(CamImage img){
    int inputLen = img.getImgSize();
    uint8_t* p = img.getImgBuff();
    Serial.println("#Image");
    while(inputLen > 0)
    {
      int len = inputLen > lineSize ? lineSize : inputLen;
      inputLen = inputLen - len;
      base64Encode(encoded, p, len); 
      p += len;
      Serial.println((char*)encoded);
    }
    Serial.println("#End");
}

void setup()
{
  Serial.begin(BAUDRATE);
  while (!Serial);
  
  initCamera();
}

void loop()
{
  // 撮影できる限界の暗さじゃないと、カメラ画像が生成されないみたい
  delay(10);
  CamImage img = theCamera.takePicture();
  Serial.println(img.getImgSize());
  if (img.isAvailable())
  {
    int iso = theCamera.getISOSensitivity();
    int ss = theCamera.getAbsoluteExposure();
    int a = int(1000 /( ss*100/1000 ));
    int hdr = theCamera.getHDR();
    Serial.print("ISO ");
    Serial.print(iso);
    Serial.print(",ss 1/");
    Serial.print(a);
    Serial.print(",HDR ");
    Serial.print(hdr);
    Serial.println();

    // 画像の転送
    digitalWrite(LED0, HIGH);
    sendImageToSerial(img);
    digitalWrite(LED0, LOW);
  }else{
  }
}
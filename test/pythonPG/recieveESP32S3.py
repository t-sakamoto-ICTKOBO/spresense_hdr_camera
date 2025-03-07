import serial
import base64
import cv2
import numpy as np
import time
import datetime
with serial.Serial('/dev/cu.usbmodem1101',115200, timeout=3) as reader:
    count = 0

    readingBase64 = False
    list = []
    while count < 100:
        line = reader.readline().decode('ascii').strip()
        
        # 画像部分を切り出す
        if line.startswith('#Image'):
            readingBase64 = True
            continue
        if line.startswith('#End'):
            buffer = base64.b64decode(''.join(list))
            list.clear()
            data = np.frombuffer(buffer, dtype=np.uint8)
            image = cv2.imdecode(data, cv2.IMREAD_COLOR)
            # cv2.imshow("Camera",image)
            cv2.waitKey(1) 
            count = count + 1
            readingBase64 = False

            now = datetime.datetime.now()
            unix = int(now.timestamp())

            cv2.imwrite(f'picture/{unix}.jpg', image)
            break

        if readingBase64:
            list.append(line)
        else:
            pass
            # print(line)
    cv2.destroyAllWindows()
import numpy as np
import cv2
from ultralytics import YOLO
import torch
import torch.nn as nn
import torch.nn.functional as F
import psycopg2
import time
from datetime import datetime


class CSDN_Tem(nn.Module):
    def __init__(self, in_ch, out_ch):
        super(CSDN_Tem, self).__init__()
        self.depth_conv = nn.Conv2d(
            in_channels=in_ch,
            out_channels=in_ch,
            kernel_size=3,
            stride=1,
            padding=1,
            groups=in_ch
        )
        self.point_conv = nn.Conv2d(
            in_channels=in_ch,
            out_channels=out_ch,
            kernel_size=1,
            stride=1,
            padding=0,
            groups=1
        )

    def forward(self, input):
        out = self.depth_conv(input)
        out = self.point_conv(out)
        return out

class enhance_net_nopool(nn.Module):

    def __init__(self, scale_factor):
        super(enhance_net_nopool, self).__init__()

        self.relu = nn.ReLU(inplace=True)
        self.scale_factor = scale_factor
        self.upsample = nn.UpsamplingBilinear2d(scale_factor=self.scale_factor)
        number_f = 32

        # zerodce DWC + p-shared
        self.e_conv1 = CSDN_Tem(3, number_f) 
        self.e_conv2 = CSDN_Tem(number_f, number_f) 
        self.e_conv3 = CSDN_Tem(number_f, number_f) 
        self.e_conv4 = CSDN_Tem(number_f, number_f) 
        self.e_conv5 = CSDN_Tem(number_f*2, number_f) 
        self.e_conv6 = CSDN_Tem(number_f*2, number_f) 
        self.e_conv7 = CSDN_Tem(number_f*2, 3) 

    def enhance(self, x, x_r):
        # Resize x to match the size of x_r
        x = F.interpolate(x, size=x_r.shape[2:], mode='bilinear', align_corners=False)
        
        x = x + x_r * (torch.pow(x, 2) - x)
        x = x + x_r * (torch.pow(x, 2) - x)
        x = x + x_r * (torch.pow(x, 2) - x)
        enhance_image_1 = x + x_r * (torch.pow(x, 2) - x)        
        x = enhance_image_1 + x_r * (torch.pow(enhance_image_1, 2) - enhance_image_1)        
        x = x + x_r * (torch.pow(x, 2) - x)    
        x = x + x_r * (torch.pow(x, 2) - x)
        enhance_image = x + x_r * (torch.pow(x, 2) - x)    

        return enhance_image
        
    def forward(self, x):
        if self.scale_factor == 1:
            x_down = x
        else:
            x_down = F.interpolate(x, scale_factor=1/self.scale_factor, mode='bilinear')

        x1 = self.relu(self.e_conv1(x_down))
        x2 = self.relu(self.e_conv2(x1))
        x3 = self.relu(self.e_conv3(x2))
        x4 = self.relu(self.e_conv4(x3))
        x5 = self.relu(self.e_conv5(torch.cat([x3, x4], 1)))
        x6 = self.relu(self.e_conv6(torch.cat([x2, x5], 1)))
        x_r = torch.tanh(self.e_conv7(torch.cat([x1, x6], 1)))
        if self.scale_factor != 1:
            x_r = self.upsample(x_r)
        
        # Ensure x and x_r have the same dimensions
        x = F.interpolate(x, size=x_r.shape[2:], mode='bilinear', align_corners=False)
        
        enhance_image = self.enhance(x, x_r)
        return enhance_image, x_r

def main():
    # PostgreSQL 데이터베이스 연결 정보
    db_params = {
        'dbname': '',
        'user': '',
        'password': '',
        'host': '',
        'port': ''
    }

    # YOLO 모델 및 저조도 향상 모델 로드
    yolo_model = YOLO('yolo11x.pt')
    DCE_net = enhance_net_nopool(12)
    DCE_net.load_state_dict(torch.load('./dce.pth', map_location=torch.device('cpu')))

    # RTSP 스트림 비디오 캡처
    cap = cv2.VideoCapture('')

    # PostgreSQL 연결 설정
    conn = psycopg2.connect(**db_params)
    cur = conn.cursor()

    feature_params = dict(maxCorners=50, qualityLevel=0.01, minDistance=30, blockSize=14)
    lk_params = dict(winSize=(15, 15), maxLevel=0, criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))

    threshold = 0.5
    person_detected = False
    bed_detected = False
    person_box = None
    last_state = "none"
    last_movement_time = time.time()  # 마지막 움직임 시간을 기록
    sleep_duration_limit = 10 * 60  # 10분 (600초)
    ret, old_frame = cap.read()

    # 초기 프레임에 저조도 향상 적용
    old_frame_tensor = torch.from_numpy(np.asarray(old_frame) / 255.0).float()
    old_frame_tensor = old_frame_tensor.permute(2, 0, 1).unsqueeze(0)
    with torch.no_grad():
        enhanced_old_frame, _ = DCE_net(old_frame_tensor)

    old_frame = enhanced_old_frame.squeeze(0).permute(1, 2, 0).cpu().numpy()
    old_frame = np.clip(old_frame * 255, 0, 255).astype(np.uint8)
    old_gray = cv2.cvtColor(old_frame, cv2.COLOR_BGR2GRAY)
    frame_skip = 5
    frame_count = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame_tensor = torch.from_numpy(np.asarray(frame) / 255.0).float()
        frame_tensor = frame_tensor.permute(2, 0, 1).unsqueeze(0)
        with torch.no_grad():
            enhanced_frame, _ = DCE_net(frame_tensor)

        enhanced_frame = enhanced_frame.squeeze(0).permute(1, 2, 0).cpu().numpy()
        enhanced_frame = np.clip(enhanced_frame * 255, 0, 255).astype(np.uint8)

        frame_count += 1

        if frame_count % frame_skip == 0:
            results = yolo_model(frame)
            person_detected = False
            bed_detected = False

            for result in results:
                for bbox in result.boxes:
                    class_id = int(bbox.cls[0])
                    confidence = bbox.conf[0]

                    if class_id == 0 and confidence > 0.5:  # 사람 탐지
                        person_box = (int(bbox.xyxy[0][0]), int(bbox.xyxy[0][1]), int(bbox.xyxy[0][2]), int(bbox.xyxy[0][3]))
                        person_detected = True
                    if class_id == 59 and confidence > 0.5:  # 침대 탐지
                        bed_detected = True

            if person_detected:
                # 사람의 움직임을 확인하기 위한 Optical Flow
                x1, y1, x2, y2 = person_box
                roi_gray = old_gray[y1:y2, x1:x2]
                p0 = cv2.goodFeaturesToTrack(roi_gray, mask=None, **feature_params)

                if p0 is not None:
                    p0[:, :, 0] += x1
                    p0[:, :, 1] += y1

                    frame_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                    p1, st, err = cv2.calcOpticalFlowPyrLK(old_gray, frame_gray, p0, None, **lk_params)

                    good_new = p1[st == 1]
                    good_old = p0[st == 1]

                    movement = []
                    for i, (new, old) in enumerate(zip(good_new, good_old)):
                        a, b = new.ravel()
                        c, d = old.ravel()
                        distance = np.sqrt((a - c) ** 2 + (b - d) ** 2)
                        movement.append(distance)

                    mean_movement = np.mean(movement) if movement else 0
                    if mean_movement > threshold:
                        current_state = "moving"
                        last_movement_time = time.time()  # 마지막 움직임 시간 갱신
                    else:
                        current_state = "sleeping" if bed_detected else "normal"

                    # 10분 이상 움직이지 않을 경우 "sleep"으로 변경
                    if current_state == "sleeping" and (time.time() - last_movement_time > sleep_duration_limit):
                        current_state = "sleep"

                else:
                    current_state = "normal"

            else:
                current_state = "none"  # 사람이 감지되지 않을 때

            # 상태 업데이트
            if current_state != last_state:
                cur.execute("INSERT INTO public.sleeping (datetime, state) VALUES (%s, %s)", (datetime.now(), current_state))
                last_state = current_state
                conn.commit()  # 트랜잭션 커밋

            # 이전 그레이스케일 프레임 업데이트
            old_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # 자원 해제
    cap.release()
    cur.close()
    conn.close()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()

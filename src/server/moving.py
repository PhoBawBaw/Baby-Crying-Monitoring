import numpy as np
import cv2
from ultralytics import YOLO
import psycopg2
from datetime import datetime

def log_movement_to_db(state):
    try:
        conn = psycopg2.connect("dbname='' user='' password='' host='' port=''")
        cur = conn.cursor()
        cur.execute("INSERT INTO public.sleeping (datetime, state) VALUES (%s, %s)", (datetime.now(), state))
        conn.commit()
        cur.close()
        conn.close()
    except Exception as e:
        print(f"Error logging to database: {e}")

def main():
    model = YOLO('yolo11n.pt')
    rtsp_url = ''
    cap = cv2.VideoCapture(rtsp_url)

    feature_params = dict(maxCorners=50, qualityLevel=0.01, minDistance=30, blockSize=14)
    lk_params = dict(winSize=(15, 15), maxLevel=0, criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))

    threshold = 0.5
    previous_state = None
    ret, old_frame = cap.read()

    if not ret:
        print("Error: Unable to read from the video stream.")
        return

    old_gray = cv2.cvtColor(old_frame, cv2.COLOR_BGR2GRAY)

    frame_skip = 10
    frame_count = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame_count += 1
        person_detected = False  # 매 프레임마다 초기화
        person_box = None        # 매 프레임마다 초기화

        if frame_count % frame_skip == 0:
            results = model(frame)

            for result in results:
                for bbox in result.boxes:
                    class_id = int(bbox.cls[0])
                    confidence = bbox.conf[0]

                    if class_id == 0 and confidence > 0.78:
                        x1, y1, x2, y2 = map(int, bbox.xyxy[0])
                        person_box = (x1, y1, x2 - x1, y2 - y1)
                        person_detected = True
                        break

                if person_detected:
                    break

            # 사람이 감지되지 않았을 때 "none" 상태 설정
            if not person_detected:
                current_state = 'none'
                if previous_state != current_state:
                    log_movement_to_db(current_state)
                    previous_state = current_state
                continue

        # 사람이 감지된 경우에만 이동 분석 수행
        if person_detected and person_box is not None:
            x, y, w, h = person_box

            roi_gray = old_gray[y:y+h, x:x+w]
            p0 = cv2.goodFeaturesToTrack(roi_gray, mask=None, **feature_params)

            if p0 is not None:
                p0[:, :, 0] += x
                p0[:, :, 1] += y

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
                    current_state = 'moving'
                else:
                    current_state = 'normal'

                # 상태가 변경될 때만 데이터베이스에 기록
                if previous_state != current_state:
                    log_movement_to_db(current_state)
                    previous_state = current_state

            old_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        k = cv2.waitKey(1) & 0xff
        if k == 27:
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()

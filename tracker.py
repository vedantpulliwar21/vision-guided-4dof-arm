"""
Vision tracker for the vision-guided 4-DOF arm.

- Reads the MJPEG stream from the ESP32-S3 camera
- Learns the empty background, then finds the largest new object in view
- Sends base/shoulder/elbow angles to the Arduino to keep the object centred

Keys: R = release object, Q = quit
Run:  python tracker.py
"""

import time
import cv2
import serial

# ---- Settings: change these ----
STREAM_URL = "http://192.168.1.50:81/stream"  # IP shown in the ESP32 Serial Monitor
SERIAL_PORT = "COM5"   # Windows: COM3/COM5..., Mac: /dev/cu.usbmodem..., Linux: /dev/ttyACM0
BAUD = 115200
MIN_AREA = 800         # ignore blobs smaller than this (pixels)
GAIN = 0.04            # how fast the arm follows; lower = smoother
SEND_EVERY = 0.05      # seconds between serial commands (~20 Hz)


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def main():
    arduino = serial.Serial(SERIAL_PORT, BAUD, timeout=0.1)
    time.sleep(2)  # Arduino resets when the port opens

    cap = cv2.VideoCapture(STREAM_URL)
    if not cap.isOpened():
        print("Could not open the camera stream. Check STREAM_URL and Wi-Fi.")
        return

    bg = cv2.createBackgroundSubtractorMOG2(history=300, varThreshold=40, detectShadows=False)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))

    base, shoulder, elbow = 90.0, 90.0, 90.0
    last_send = 0.0
    print("Learning background: keep the scene empty for ~3 seconds...")

    while True:
        ok, frame = cap.read()
        if not ok:
            continue
        h, w = frame.shape[:2]

        # 1. Foreground mask: whatever stands out from the learned background
        mask = bg.apply(frame)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
        mask = cv2.dilate(mask, kernel, iterations=2)
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        if contours:
            c = max(contours, key=cv2.contourArea)
            if cv2.contourArea(c) > MIN_AREA:
                x, y, bw, bh = cv2.boundingRect(c)
                cx, cy = x + bw // 2, y + bh // 2
                cv2.rectangle(frame, (x, y), (x + bw, y + bh), (0, 255, 0), 2)
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)

                # 2. Proportional control: pixel error from image centre -> joint angles
                base = clamp(base - GAIN * (cx - w / 2), 0, 180)
                shoulder = clamp(shoulder + GAIN * (cy - h / 2), 15, 165)
                elbow = clamp(180 - shoulder, 20, 160)  # keeps the claw roughly level

                # 3. Send "base,shoulder,elbow" to the Arduino
                now = time.time()
                if now - last_send > SEND_EVERY:
                    arduino.write(f"{int(base)},{int(shoulder)},{int(elbow)}\n".encode())
                    last_send = now

        # Show any status messages from the Arduino (GRABBED / RELEASED)
        if arduino.in_waiting:
            msg = arduino.readline().decode(errors="ignore").strip()
            if msg:
                print("Arduino:", msg)

        cv2.imshow("Tracking", frame)
        cv2.imshow("Mask", mask)
        key = cv2.waitKey(1) & 0xFF
        if key == ord("r"):
            arduino.write(b"R\n")
        elif key == ord("q"):
            break

    cap.release()
    arduino.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

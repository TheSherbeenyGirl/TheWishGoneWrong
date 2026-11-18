import cv2
import mediapipe as mp
import socket

# ==========================================
# UDP CONNECTIONS
# ==========================================

hand_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
camera_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

HAND_ADDRESS = ("127.0.0.1", 5005)
CAMERA_ADDRESS = ("127.0.0.1", 5006)

# ==========================================
# CAMERA
# ==========================================

camera = cv2.VideoCapture(0)

if not camera.isOpened():
    print("Could not open camera")
    exit()

camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# ==========================================
# MEDIAPIPE
# ==========================================

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    max_num_hands=2,
    model_complexity=1,
    min_detection_confidence=0.7,
    min_tracking_confidence=0.7
)

print("The Wish Gone Wrong - AR Sender")
print("Sending hand data to port 5005")
print("Sending camera frames to port 5006")
print("Press Ctrl+C to stop.")

# ==========================================
# MAIN LOOP
# ==========================================

try:
    while True:

        success, frame = camera.read()

        if not success:
            continue

       # frame = cv2.flip(frame, 1)
        frame = cv2.resize(frame, (640, 480))

        height, width, _ = frame.shape

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb_frame)

        # Default hand values
        left_present = 0
        left_x, left_y = -1.0, -1.0

        right_present = 0
        right_x, right_y = -1.0, -1.0

        # ==================================
        # HAND DETECTION
        # ==================================

        if results.multi_hand_landmarks:

            for i, hand_landmarks in enumerate(
                results.multi_hand_landmarks
            ):

                label = results.multi_handedness[i].classification[0].label

                palm_points = [0, 5, 9, 13, 17]

                center_x = sum(
                    hand_landmarks.landmark[p].x
                    for p in palm_points
                ) / len(palm_points)

                center_y = sum(
                    hand_landmarks.landmark[p].y
                    for p in palm_points
                ) / len(palm_points)

                if label == "Left":
                    left_present = 1
                    left_x, left_y = center_x, center_y
                else:
                    right_present = 1
                    right_x, right_y = center_x, center_y

                mp_draw.draw_landmarks(
                    frame,
                    hand_landmarks,
                    mp_hands.HAND_CONNECTIONS
                )

                px = int(center_x * width)
                py = int(center_y * height)

                cv2.circle(
                    frame,
                    (px, py),
                    10,
                    (0, 0, 255),
                    -1
                )

                cv2.putText(
                    frame,
                    f"{label} Hand",
                    (px + 15, py),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (0, 255, 0),
                    2
                )

        # ==================================
        # SEND HAND COORDINATES
        # ==================================

        hand_data = (
            f"{left_present},{left_x:.4f},{left_y:.4f},"
            f"{right_present},{right_x:.4f},{right_y:.4f}"
        )

        hand_sock.sendto(
            hand_data.encode("utf-8"),
            HAND_ADDRESS
        )

        # ==================================
        # SEND CAMERA FRAME
        # ==================================

        success, encoded_frame = cv2.imencode(
            ".jpg",
            frame,
            [cv2.IMWRITE_JPEG_QUALITY, 45]
        )

        if success:
            frame_data = encoded_frame.tobytes()

            # Keep the UDP packet within a safe size
            if len(frame_data) <= 60000:
                camera_sock.sendto(
                    frame_data,
                    CAMERA_ADDRESS
                )

except KeyboardInterrupt:
    print("\nStopping AR sender...")

finally:
    hands.close()
    camera.release()

    hand_sock.close()
    camera_sock.close()

    print("Camera and sockets closed.")

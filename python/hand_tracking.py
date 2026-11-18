import cv2
import mediapipe as mp
import math

camera = cv2.VideoCapture(0)

if not camera.isOpened():
    print("Could not open camera")
    exit()

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    max_num_hands=2,
    model_complexity=1,
    min_detection_confidence=0.7,
    min_tracking_confidence=0.7
)

print("Hand tracking started!")
print("Press Q to quit.")

while True:

    success, frame = camera.read()

    if not success:
        break

    frame = cv2.flip(frame, 1)

    height, width, _ = frame.shape

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(rgb_frame)

    if results.multi_hand_landmarks:

        for i, hand_landmarks in enumerate(results.multi_hand_landmarks):

            # Identify left or right hand
            handedness = results.multi_handedness[i]
            hand_label = handedness.classification[0].label

            # Draw hand skeleton
            mp_draw.draw_landmarks(
                frame,
                hand_landmarks,
                mp_hands.HAND_CONNECTIONS
            )

            # Calculate palm center using palm landmarks
            palm_points = [0, 5, 9, 13, 17]

            center_x = sum(
                hand_landmarks.landmark[p].x for p in palm_points
            ) / len(palm_points)

            center_y = sum(
                hand_landmarks.landmark[p].y for p in palm_points
            ) / len(palm_points)

            # Convert normalized coordinates to pixels
            pixel_x = int(center_x * width)
            pixel_y = int(center_y * height)

            # Draw palm center
            cv2.circle(
                frame,
                (pixel_x, pixel_y),
                12,
                (0, 0, 255),
                -1
            )

            # Display hand label and coordinates
            cv2.putText(
                frame,
                f"{hand_label} Hand",
                (pixel_x + 15, pixel_y - 15),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 255, 0),
                2
            )

            cv2.putText(
                frame,
                f"X: {center_x:.2f} Y: {center_y:.2f}",
                (pixel_x + 15, pixel_y + 15),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (255, 255, 255),
                2
            )

            print(
                f"{hand_label}: X={center_x:.2f}, Y={center_y:.2f}   ",
                end="\r"
            )

    cv2.imshow("The Wish Gone Wrong - Hand Tracking", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

hands.close()
camera.release()
cv2.destroyAllWindows()

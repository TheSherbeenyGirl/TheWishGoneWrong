## The Wish Gone Wrong

An Augmented Reality simulation and interactive mini-game experience inspired by the Egyptian movie **طير إنت**.

The project aims to recreate the movie's magical and humorous atmosphere through a collection of interactive AR scenes. Each scene will have its own theme, environment, mechanics, and mini-game.

Users will interact with the virtual environment using hand gestures, turning the experience into a more immersive form of human-computer interaction.

---

## 🎮 Project Concept

The experience is divided into multiple themed scenes.

Each scene will contain:
- A unique environment inspired by the movie.
- Interactive 3D objects and visual effects.
- A small game or challenge.
- Gesture-based interactions.
- A beginning, objective, and completion condition.

The scenes will eventually connect to form one complete experience.

---

## Technologies

| Technology | Purpose |
|---|---|
| C++ | Main application and interaction logic |
| OpenGL | Real-time 3D rendering |
| GLFW | Window management |
| GLAD | OpenGL function loading |
| GLM | 3D mathematics |
| Assimp | 3D model importing |
| Python | Computer vision processing |
| OpenCV | Camera capture and image processing |
| MediaPipe | Hand landmark detection |
| UDP / Networking | Communication between Python and C++ |
| CMake | Build system |

---

##Current Project Structure

```text
TheWishGoneWrong/
│
├── assets/
│   └── models/
│       └── lamp.glb
│
├── include/
│
├── src/
│   └── model_viewer.cpp
│
├── python/
│   └── hand_sender.py
│
├── CMakeLists.txt
├── README.md
└── .gitignore
```

---

# Development Progress

## Phase 1 — Technical Prototyping
**Status: In progress**

The purpose of this phase is to test the technologies and establish the foundation before building the actual scenes.

### 3D Rendering
- [x] Set up OpenGL.
- [x] Configure GLFW and GLAD.
- [x] Integrate GLM.
- [x] Load 3D models using Assimp.
- [x] Load model textures and materials.
- [ ] Finalize model orientation and camera perspective.

### Computer Vision
- [x] Set up Python environment.
- [x] Integrate OpenCV.
- [x] Integrate MediaPipe.
- [x] Detect hand landmarks.
- [x] Test pinch detection.
- [ ] Reliable two-hand tracking.
- [ ] Implement gesture recognition.

### Camera Integration
- [x] Capture webcam frames.
- [x] Stream camera frames to the C++ application.
- [x] Display the camera feed as the application background.
- [ ] Improve camera resolution and streaming quality.
- [ ] Replace the laptop webcam with a mobile phone camera.
- [ ] Transfer the mobile camera feed over Wi-Fi.
- [ ] Optimize latency and frame stability.

### Communication
- [x] Establish Python–C++ communication.
- [x] Transfer hand tracking data through UDP.
- [x] Transfer camera frames through UDP.
- [ ] Refine the communication architecture.
- [ ] Synchronize camera frames and hand tracking data.

---

## Phase 2 — Gesture Recognition System
**Status: Upcoming**

Develop a reliable interaction system that recognizes hand gestures and maps them to actions.

Planned work:
- Detect and track two hands independently.
- Recognize static hand poses.
- Recognize dynamic gestures.
- Distinguish between left and right hands.
- Implement gesture states and transition handling.
- Reduce accidental gesture activation.
- Create a reusable gesture-to-action system.

---

## Phase 3 — AR Environment & Interaction Framework
**Status: Upcoming**

Build the foundation for creating multiple interactive scenes.

- Create a reusable scene management system.
- Implement scene transitions.
- Develop reusable object interaction mechanics.
- Implement gesture-controlled object manipulation.
- Add collision detection.
- Add animations and particle effects.
- Establish a consistent visual style.

---

## Phase 4 — Themed Scenes & Mini-Games
**Status: Planned**

Create multiple scenes inspired by the movie.

Each scene will have:
- Its own environment and atmosphere.
- Unique 3D assets.
- A specific objective.
- A mini-game or interactive challenge.
- Scene-specific gesture controls.
- A scoring or completion system where appropriate.

The exact scenes and their mechanics will be designed and implemented as development progresses.

---

## Phase 5 — Audio & Immersion
**Status: Planned**

- Add sound effects.
- Add background music.
- Implement voice interaction where needed.
- Add visual feedback for user actions.
- Synchronize sound and animations.
- Improve the overall atmosphere of each scene.

---

## Phase 6 — Integration & Optimization
**Status: Planned**

- Connect all scenes into one experience.
- Implement the complete experience flow.
- Optimize rendering performance.
- Reduce camera and gesture latency.
- Test different lighting conditions.
- Improve tracking reliability.
- Prepare the final demonstration.

---

## Final Goal

Create a collection of interactive AR scenes inspired by **طير إنت**, combining:

- Augmented Reality.
- Real-time 3D graphics.
- Computer vision.
- Hand gesture recognition.
- Interactive mini-games.
- Audio and visual effects.

The goal is not simply to recreate scenes from the movie, but to turn its themes and atmosphere into an interactive experience.

---

**Current focus:** Technical prototyping — mobile camera streaming, Wi-Fi communication, reliable hand tracking, and gesture recognition.

*Work in progress. The adventure is just beginning.* 🪔

# vision-guided-4dof-arm
4-DOF robotic arm with ESP32-CAM object tracking and ultrasonic proximity-triggered grasping
# Vision-Guided 4-DOF Manipulator


A low-cost robotic arm that locates objects with a camera and grips
only when they're in reach, instead of moving to preset positions.

## How it works
- **Vision:** ESP32-CAM streams frames to an OpenCV pipeline that
  localizes and tracks the target.
- **Grasp trigger:** An ultrasonic sensor closes the gripper when the
  object is within 3 cm.
- **Mechanics:** 4 DOF, servos, fully 3D printed in [PLA].

## Hardware
| Part | Qty |
|------|-----|
| ESP32-CAM | 1 |
| MG90S SERVO MOTORS | 4 |
| HC-SR04 ultrasonic sensor | 1 |
| Power supply PCA 9685 | 1 |

## Design iterations
Joint tolerances and cable routing were revised across print versions
to reduce backlash and improve pick-and-place repeatability.

## Repo structure
- `firmware/`: ESP32 code
- `vision/`: OpenCV tracking scripts
- `cad/`: STEP and STL files
- `docs/`: wiring diagram and BOM

## Author
Vedant Pulliwar | [LinkedIn](https://linkedin.com/in/vedantpulliwar)

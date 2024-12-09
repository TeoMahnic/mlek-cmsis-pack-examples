# Object Detection - Performance

Average time consumed by application code section, measured using [EventRecorder:EventStatistic](https://arm-software.github.io/CMSIS-View/latest/scvd_evt_stat.html) is in table below.

|                                             | **prepare frame for ML** | **pre-processing** | **inference** | **post-processing** | **display frame with results** |
|---------------------------------------------|--------------------------|--------------------|---------------|---------------------|--------------------------------|
| **Alif-Appkit-E7-HP + Ethos U55** (400 MHz) | 20.3ms                   | 15.1ms             | 3.67ms        | 0.495ms             | 2.25ms                         |
| **Alif-Appkit-E7-HP** (400MHz)              | 20.3ms                   | 30ms               | 810ms         | 0.500ms             | 2.25ms                         |

## Description of code sections
- **prepare frame for ML**  
  capture frame with camera, perform debayering step and crop frame to requested size
- **display frame with results**  
  copy processed frame with results from application buffer to internal display buffer
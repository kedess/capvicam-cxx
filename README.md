# capvicam-cxx
Utility for capturing video from a USB camera via the video for linux system (v4l2)

Build
```bash
git clone https://github.com/kedess/capvicam.git
cd capvicam
mkdir build
cd build
cmake ..
make -j4
```

Watching videos in the browser:
```bash
http://localhost:8000
```

Watching videos via ffplay:
```bash
ffplay http://localhost:8000
```
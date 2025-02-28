## Unreleased

### BREAKING CHANGE

- changed proto file
- Many changes at NetworkController

### Feat

- version with follow waypoints & show pose and map
- **GUI**: added logo, removed speech from menu & added map, robot info entries
- added dialog & widget for showing global map & robot. added robot info dialog
- **protos**: splitted proto files & added global map recieving with robot pose
- added lookup table for robots with storing in database, grpc build related fixes & added config controller

### Fix

- changed raw pointers to shared_ptr
- **speech**: removed unused speech processor

## 0.2.0 (2023-04-30)

### Feat

- **NetworkController**: Add finding robots by broadcast request
- **NetworkController**: Added first arp message via JSON

### Fix

- **CMakeLists**: Fix some problems with compiling SpeechProccessor
- **NetworkController**: Removed forgotten include to nonexisted file
- **AmurCore.pro**: Fix gRPC linking error & add commitizen config
- **CamCalibrate**: Fixed waiting new frame with chessboard, changed to std::chrono lib
- **JSONWorker**: Disable JSONWorker, for possible building

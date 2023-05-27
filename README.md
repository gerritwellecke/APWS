# Automatic Plant Watering System
This is a simple system based on the ESP8266 (in my case a nodeMCUv2 development board) which will keep my plants watered whenever I'm not home.

## Version 0.1
**Goal:** bare minimum running system consisting of:
- Up to 4 plants being watered
- Regular intervalls, defaulting to something like watering once per week
- Rough setting for the amount of water each plant will receive

### Future Plans
- Use moisture sensors for each plant
- Data log for each plant
- Water sensor to warn, when reservoir needs to be filled
- Push notifications using Telegram bot

## Code Style 
Use `llvm` code style but with up to 88 columns. 
This can be achieved with, e.g. `clang-format --style='{ColumnLimit: 88}'`.

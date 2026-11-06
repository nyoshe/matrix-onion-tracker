SKETCH := matrix-onion-tracker
# ELEGOO ESP-WROOM-32 dev board = generic "ESP32 Dev Module", 4 MB flash, no PSRAM.
# huge_app: a single 3 MB app partition instead of the default's two 1.25 MB
# OTA slots + an unused 1.5 MB SPIFFS -- we flash over USB, not OTA, and
# store data in NVS (Preferences), not a filesystem.
FQBN   ?= esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashSize=4M,PSRAM=disabled,PartitionScheme=huge_app
PORT   ?= /dev/ttyUSB0
BAUD   ?= 115200
BUILD  := build

CORE := esp32:esp32@3.3.12
LIBS := \
	"ESP32 HUB75 LED MATRIX PANEL DMA Display@3.1.0" \
	"Adafruit GFX Library@1.13.0" \
	"ArduinoJson@7.4.3"

.PHONY: setup build upload monitor flash clean

setup:
	arduino-cli core update-index
	arduino-cli core install $(CORE)
	arduino-cli lib install $(LIBS)

build:
	arduino-cli compile --fqbn $(FQBN) --build-path $(BUILD) $(SKETCH)

upload: build
	arduino-cli upload --fqbn $(FQBN) --port $(PORT) --input-dir $(BUILD) $(SKETCH)

monitor:
	arduino-cli monitor --port $(PORT) --config baudrate=$(BAUD)

flash: upload monitor

clean:
	rm -rf $(BUILD)

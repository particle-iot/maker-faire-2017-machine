FIRMWARE_VERSION=0.5.1-pi.14
DIR=$(shell pwd)
FIRMWARE=$(HOME)/Programming/firmware-rpi
compile:
	particle compile raspberrypi . --saveTo firmware.bin --target $(FIRMWARE_VERSION)

precompile:
	docker run --rm -v "$(DIR):/input" -v "$(DIR):/output" particle/buildpack-wiring-preprocessor

compile-debug: precompile
	docker run --rm -v "$(FIRMWARE):/firmware" -v "$(DIR):/input" -v "$(DIR):/output" particle/buildpack-raspberrypi make all STRIP_SYMBOLS=n

flash:
	particle flash hero . --target $(FIRMWARE_VERSION)

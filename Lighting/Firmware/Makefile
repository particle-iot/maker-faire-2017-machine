all: compile flash

compile:
	particle compile photon . --saveTo photon_firmware.bin --target 0.6.1

flash:
	particle flash --usb photon_firmware.bin

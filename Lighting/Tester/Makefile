all: compile flash

compile:
	particle compile electron . --saveTo electron_firmware.bin --target 0.6.1

flash:
	particle flash --usb electron_firmware.bin

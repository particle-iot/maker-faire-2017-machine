Some firmware libraries to use in the Maker Faire Machine modules.

## Simple Storage

For loading and storing parameters in the EEPROM. See the file `examples/basic/basic.ino`

For each panel, use app code with the panel number, i.e. `APP_CODE('M', 'F', 0, 2)` for second panel.

## Communication

For sharing state between multiple modules through the CAN bus.

## Beam Detector

Ball detector using a laser break beam

## HSV

Convert between red/green/blue colors and hue/saturation/value colors.

## Encoder

Count pulses from a quadrature rotary encoder.

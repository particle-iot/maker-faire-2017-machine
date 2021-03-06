# Ball Detector

## Parts

- Bracket (designed with Fusion 360) http://a360.co/2oVDeuF
- Round bracket with laser in the middle (designed with Fusion 360) http://a360.co/2qUFsPa		
- Round bracket with laser offset (designed with Fusion 360) http://a360.co/2q6rvfz
- Laser Diode https://www.amazon.com/gp/product/B01ITK4PEO
- Photoresistor https://www.amazon.com/gp/product/B016D737Y4

## Circuit

<img src="Circuit diagram_schem.png">

Open points / suggestions:
- Use a potentiometer instead of a fixed pull down resistor for the photoresistor to allow calibrating the edges on site. 2 kOhm potentiometer suggested.
- Add a low-pass RC filter to the light level pin to filter out high frequency noise.

## Software Library

https://github.com/spark/library-beam-detector

## Assembly

<img src="assembled.jpg">

- 3d print the bracket. I used 0.2 mm layer height, 5% rectilinear infill.
- Insert the laser diode loosely. If the laser diode is too loose or doesn't fit in the hole,
  tweak the diameter in Fusion 360 and print again. If it is a tad too
  tight, manually turn a 15/64" drill bit in the hole to remove some of
  the imperfections from the print.
- Insert the photoresistor. I like cutting one leg shorter than the
other so I can insert them one at at time in the tiny holes.
- Connect a multimeter to the photoresistor in resistance measurement mode.
- Connect a power supply (3.3V or 5V) to the laser diode. Blue is ground, red is positive.
- Rotate the laser diode in the hole to get the smallest resistance measurement on the photoresistor.
- Press the laser diode firmly in.
- Solder some wires to the photoresistor wires and shove the wires inside the channel.
- Unsolder the laser diode wires and solder some sturdier wires.

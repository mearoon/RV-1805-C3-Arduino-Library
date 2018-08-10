# Arduino Library for RV-1805-C3

Supports Micro Crystal [RV-1805-C3](http://www.microcrystal.com/images/_Product-Documentation/02_Oscillator_&_RTC_Modules/01_Datasheet/RV-1805-C3.pdf) extreme low power RTC module.

## Usage

Search the library using the Library Manager of Arduino IDE, or download it directly via GitHub.

Connect an RV-1805-C3 device to your Arduino board according to the datasheet. There are three examples that come with the library:

- `SetDateTime`: Configures date and time to the RTC module via the serial console, and then
  prints the current date and time in ISO 8601 format every second.
- `AlarmInterrupt`: Sets an alarm based on calendar date and time, and triggers an interrupt.
- `CountdownTimer`: Sets a repeating countdown timer that triggers an interrupt.

You can find more API functions in `src/RV1805C3.h`, which are pretty much self-explanatory if you have read the datasheet.

## Compatibility

This library has been tested on ESP8266 only. However, it should also be compatible with any architecture that provides `Wire.h` I<sup>2</sup>C support.

## License

MIT

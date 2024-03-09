import time
from typing import List

import spidev


COLORS_PER_LED = [
    "0x000000",
    "0x0000FF",
]

COLOR_CIRCLE = [
    "0x0000ff","0x1700e8","0x2e00d1","0x4600b9","0x5d00a2","0x74008b","0x8b0074",
    "0xa2005d","0xb90046","0xd1002e","0xe80017","0xff0000","0xe81700","0xd12e00",
    "0xb94600","0xa25d00","0x8b7400","0x748b00","0x5da200","0x46b900","0x2ed100",
    "0x17e800","0x00ff00","0x00e817","0x00d12e","0x00b946","0x00a25d","0x008b74",
    "0x00748b","0x005da2","0x0046b9","0x002ed1","0x0017e8",
]


class WS2812:
    TICK = 0.15e-6
    LED_COUNT = 8

    CODE_0 = [0b11000000]
    CODE_1 = [0b11111000]

    delay = 0.04

    def __init__(self) -> None:
        self.spi = spidev.SpiDev()
        self.freq = int(1 / self.TICK)
        print(f"Calculated frequency: {self.freq:_} MHz")

    def __enter__(self):
        self.spi.open(0, 0)
        self.spi.lsbfirst = False
        self.spi.cshigh = False
        self.spi.mode = 0b00
        self.spi.bits_per_word = 8
        self.spi.max_speed_hz = self.freq
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.spi.close()

    @classmethod
    def bits8(cls, hex):
        return bin(hex)[2:].zfill(8)

    @classmethod
    def split_colorcode(cls, colorcode):
        if isinstance(colorcode, int):
            colorcode = str(hex(colorcode))
        if colorcode.startswith("0x"):
            colorcode = colorcode[2:]

        rgb = []
        for i in (2, 0, 4):
            decimal = int(colorcode[i:i+2], 16)
            rgb.append(decimal)
        return tuple(rgb)

    def set(self, colors_per_led: List[str]):
        code = []
        for cpl in colors_per_led:
            for color in self.split_colorcode(cpl):
                color_bits = self.bits8(color)
                for bit in color_bits:
                    if bit == "1":
                        code += self.CODE_1
                    elif bit == "0":
                        code += self.CODE_0
                    else:
                        raise ValueError

        self.spi.xfer(code)

    # TODO: Add direction argument
    # TODO: Add color settings
    def running_light(self):
        current_led = 0
        current_color = 0
        dir_right = True
        try:
            while True:
                colors_per_led = ["0x000000"] * self.LED_COUNT
                colors_per_led[current_led] = COLOR_CIRCLE[current_color]
                self.set(colors_per_led)
                time.sleep(self.delay)

                if current_led >= self.LED_COUNT - 1:
                    dir_right = False
                    current_color += 1
                if current_led <= 0:
                    dir_right = True
                    current_color += 1

                if dir_right:
                    current_led += 1
                else:
                    current_led -= 1

                if current_color >= len(COLOR_CIRCLE):
                    current_color = 0
        except KeyboardInterrupt:
            pass


def main():
    with WS2812() as leds:
        leds.running_light()


if __name__ == "__main__":
    main()

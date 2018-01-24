# 2m-Radio
2m/VHF Radio based on DRA818V, with a small oled i2c display

Current dev board is based on [MattairTech](https://github.com/mattairtech) [MT-D21E](https://www.mattairtech.com/index.php/development-boards/mt-d21e.html) Rev A board.
This board uses the Atmel ATSAMD21E18A chip.

The RF module is the [Dorji DRA818V](http://www.dorji.com/products-detail.php?ProId=55), which can be had [fairly 
cheap](https://www.ebay.com/itm/144MHz-145MHz-146MHz-154MHz-174MHz-VHF-HAM-Radio-Module-Amateur-Radio-DRA818V/191210888951) (~$12)

The display is a [White 1.3" 128x64 SSH1106 based I2C OLED display](https://www.ebay.com/itm/1-3-White-OLED-LCD-4PIN-Display-Module-IIC-I2C-Interface-128x64-for-Arduino/382325318585) off eBay.

The display is being driven with the [u8g2 library](https://github.com/olikraus/u8g2).

[Current look at the physical side](https://i.imgur.com/4F7A7NM.jpg).


My plan is to eventually have a 2m radio that I can use with an external amp to hit some local repeaters (giving me a 2m radio in the house).
Also, improving my coding and github skills. Never hurts to have a project going.


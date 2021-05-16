# BLEMidiMETROland
Display for metronome from Roland FP30 / FP10 with ESP32 BLE.

YouTube video here: https://www.youtube.com/watch?v=GZcshRPVRXY.

First of all many thanks to Drahoslav Bednář from https://pian.co/ (https://github.com/drahoslove/pianco). Without his support I would not be able to develop this code.

I have an Roland FP-30 which I love.
One thing that I missed in this piano is an indication of the rythm of the metronome (without having to use the Piano Partner 2 app). So I decided to make use of the Bluetooth connection with FP-30 and get the beat using an ESP32 and show it in a display.

Without having many experience with a display I picked a LILYGO® TTGO T5 V2.3 2.13 Inch E-Paper Screen (http://www.lilygo.cn/prod_view.aspx?TypeId=50031&Id=1149&FId=t3:50031:3, https://github.com/Xinyuan-LilyGO/LilyGo-T5-ink-series) because it just looks very nice. If I would have to pick a screen now I would probably choose something else because of the refresh limitations of e-paper screens.

For the BLE connection I used NimBLE (https://github.com/h2zero/NimBLE-Arduino/). Many thanks to the developers. My code is based on the NimBLE_Client example.

I 3D printed a case for the ESP32 from https://www.thingiverse.com/thing:4670205. Thank you for the design from xl0e. This case fits the LILYGO® TTGO T5 very well. The only thing that I would change is the access to the buttons from the ESP32. It is not easy to reach them without a paper clip or something like that. In the future I would like to make use of the button from the LILYGO® TTGO T5 ESP32 to cycle through different metronome beats.

Once you turn on the ESP32 it will search for the FP-30 and try to connect. If the connection us successfull it will show a Bluetooth symbol in the top right corner. It will also set the beat to 60 and you will see 60 on the display. If you increase or decreast the beat you will see it changing on the display.

I decided to share the code so that others can use it an improve it.

Now have some fun a go practise your Hanon!

jcgfsantos

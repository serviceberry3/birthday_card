### Touch keyboard birthday card ###
This is a card that I made for someone's birthday. I unfortunately had to cut out a good amount of the code that I had originally; I seemed to be having
trouble getting it to fit into the memory of the Arduino (Elegoo) Nano.    

The interface features a volume control dial and an OLED display with three menu options. The first option is used to play the Happy Birthday song
(with karaoke-style lyric printing), the second is used to set the speed at which the song plays, and the third option, when clicked, allows the user
to play a "keyboard" of the Happy Birthday song's notes by touching the foil hearts. Each heart is attached to a resistor. The heart node side of each resistor
is connected to one of several digital pins on the Nano, and the other side of each resistor is connected to a common node which is attached to one digital pin.
To determine whether the heart is being touched, the common node is set high, and the time it takes for each heart node to read high is recorded. When the user's finger is extremely close to the heart, they form a capacitor with the heart, which adds significant delay to the propogation of the high voltage.   

You can watch a demo video here: https://drive.google.com/file/d/1f4H3VtyIXpJ8bo3nECYHF-hGAHf_pA5F/view?usp=sharing. Since I used the Arduino IDE for this
project, I decided to keep all code in one file to make things work better with the IDE.
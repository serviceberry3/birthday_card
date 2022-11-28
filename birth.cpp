#include "birth.h"

//the string representing the song
char notes[] = "GGAGcB GGAGdc GGxecBA yyecdc";

//array indicating for how long each note should (relatively) be held (including rests)
int note_hold_lengths[] = {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16};

int play() {
    //iterate through each note in song string
  for (int i = 0; i < NUM_NOTES; i++) {
    //if it's a space, just delay   
    if (notes[i] == ' ') {
        delay(note_hold_lengths[i] * TEMPO); // delay between notes
    } 

    //if there's a note that needs to be played, play given note for its intended duration
    else {
        playNote(notes[i], note_hold_lengths[i] * TEMPO);
    }

    //delay between notes
    delay(TEMPO);
  }
}


// Musical note frequencies (in Hz)
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880

#define REST 0
#define BUZZER_PIN 10

// Happy Birthday melody
int happyBirthdayMelody[] = {
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_C5, NOTE_B4,  // Happy birthday to you
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D5, NOTE_C5,  // Happy birthday to you
  NOTE_G4, NOTE_G4, NOTE_G5, NOTE_E5, NOTE_C5, NOTE_B4, NOTE_A4,  // Happy birthday dear [name]
  NOTE_F5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_C5   // Happy birthday to you
};

// Note durations: 4 = quarter note, 8 = eighth note, etc.
int happyBirthdayDurations[] = {
  8, 8, 4, 4, 4, 2,     // Happy birthday to you
  8, 8, 4, 4, 4, 2,     // Happy birthday to you  
  8, 8, 4, 4, 4, 4, 2,  // Happy birthday dear [name]
  8, 8, 4, 4, 4, 2      // Happy birthday to you
};

void playHappyBirthday() {
  // Calculate the number of notes
  int noteCount = sizeof(happyBirthdayMelody) / sizeof(happyBirthdayMelody[0]);
  
  // Iterate through all notes
  for (int i = 0; i < noteCount; i++) {
    // 0.7x speed: base duration 1000ms / 0.7 = ~1428ms
    int noteDuration = 1428 / happyBirthdayDurations[i];
    
    // Play the note
    tone(BUZZER_PIN, happyBirthdayMelody[i], noteDuration);
    
    // Add a small pause between notes
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    
    // Stop the tone
    noTone(BUZZER_PIN);
  }
}


/*
 * 
 * This program is written for Arduino UNO
 * by Ansary Tanvir.
 * 
 */

#include <SPI.h>                // include Arduino SPI library.
#include <SD.h>                 // include Arduino SD library.
#include "TMRpcm.h"             // include TMRpcm  library.


TMRpcm    audio;                /* Creating an global instance of the TMRpcm class */
const int SD_ChipSelectPin = 4; /* Self Explanatory */

/* Ultrasonic Sensor */
const int trigger   = 2;        /* Trigger pin to digital pin 2  */
const int echo      = 3;        /* Receiver pin to digital pin 3 */
int   currentVolume = 5;        /* Hold the current volume of the music */

/* IR Proximity Sensor */
const int leftSensor  = 6;      /* Left  Proximiy IR Sensor output pin to digital oin 6 */
const int rightSensor = 5;      /* RIght Proximiy IR Sensor output pin to digital oin 6 */


/**
 * A class representing a collection of music tracks.
 */
class Music {

  public:
    
    String* names;        // an array of music track names
    size_t size;          // the maximum number of tracks that can be stored
    uint8_t iterator;     // the index of the currently selected track
    uint8_t musicCounter; // the number of tracks currently stored

    bool next;            // a flag indicating if the "next" track should be played
    bool previous;        // a flag indicating if the "previous" track should be played

    /**
     * Constructor for the Music class.
     * @param size the maximum number of tracks that can be stored
     */
    Music(const size_t& size) {
      
      this->size    = size;
      iterator      = 0;
      musicCounter  = 0;
      next          = false;
      previous      = false;
      names         = new String[size];
    }

    /**
     * Destructor for the Music class.
     */
    ~Music() {
      delete names;
    }

    /**
     * Returns the name of the currently selected music track.
     * 
     * @return the name of the currently selected music track
     */
    String getCurrentMusic() {
      iterator %= musicCounter;
      return names[iterator];
    }

    /**
     * Returns the name of the next music track in the collection.
     * 
     * @return the name of the next music track in the collection
     */
    String getNextMusic() {

      next = true;
      if (previous)
        iterator++, previous = false;

      if (iterator == 0)
        return names[iterator++];
      
      iterator %= musicCounter;
      return names[iterator++];
    }

    /**
     * Returns the name of the previous music track in the collection.
     * 
     * @return the name of the previous music track in the collection
     */
    String getPreviousMusic() {

      previous = true;
      if (next)
        iterator--, next = false;

      if (iterator <= 0)
        iterator = musicCounter - 1;
      else
        iterator--;
          
      return names[iterator];
    }

    /**
     * Returns true if the given string represents a valid music track (i.e., has a ".wav" file extension).
     * @param string the string to check
     * @return true if the given string represents a valid music track, false otherwise
     */
    static const bool isMusic(const String& string) {
      
      const uint8_t nameSize = string.length();
      const String extension = string.substring(nameSize - 4);
   
      if (!extension.equalsIgnoreCase(".wav"))
        return false;

      return true;
    }

    /**
     * Adds the given music track to the collection.
     * @param musicName the name of the music track to add
     * @return true if the track was added successfully, false otherwise
     */
    const bool addMusic(const String& musicName) {

      if (!isMusic(musicName))
        return false;

      if (musicCounter >= size - 1)
        return false;

      names[musicCounter++] = musicName;
      return true;
    }

    /**
     * Prints the names of all music tracks in the collection to the serial console.
     */
    void displayMusics() {
      
      for (int i = 0; i < musicCounter; i++) {
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.print(names[i]);
        Serial.println("");
      }
      Serial.println("");
    }
};

/* Create a global instance of music class to hold atmost 8 tracks */
Music music = Music(8);
    
/**
 * The setup function for the program. Initializes the SD card, TMRpcm library, and
 * pins for sensors and ultrasonic distance sensor. It also loads music files from
 * the SD card and starts playing the first track.
 */
void setup(void) {
  
  Serial.begin(115200);
  Serial.print("Initializing SD card...");
  
  /**
   * Attempt to initialize the SD card using the SD library. If initialization fails,
   * print a debug message and enter an infinite loop.
   */
  if ( !SD.begin(SD_ChipSelectPin) ) {
    Serial.println("Failed to initialize SD Card!");
    while(true);
  }
  
  Serial.println("OK!");
  
  /**
   * Configure the TMRpcm library by setting the speaker output pin, volume, and
   * quality parameters.
   */
  audio.speakerPin = 9;
  audio.setVolume(5);
  audio.quality(1);

  /**
   * Load music files name from the SD card and store them in the "music" object.
   */
  getMusics(music);

  /**
   * Configure pins for the ultrasonic distance sensor.
   */
  {
    pinMode(trigger, OUTPUT);
    pinMode(echo, INPUT);

    // Due to pin shortage, for this project we had to use pin 8 at 5V source
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
  }

  /**
   * Configure pins for the left and right sensors.
   */
  {
    pinMode(leftSensor, INPUT);
    pinMode(rightSensor, INPUT);
  }

  /**
   * Start playing the first track in the playlist using the TMRpcm library.
   */
  playNextTrack(audio);
}


/**
 * The main loop function for the program. Checks for input from sensors and updates
 * the volume of the audio playback based on the distance to an obstacle.
 */
void loop() {

   /**
    * If the left sensor is activated, print a debug message and play the next track
    * in the playlist using the TMRpcm library.
    */
   if ( digitalRead(leftSensor) == LOW ){
      Serial.println("Left Sensor Activated");
      playNextTrack(audio);
   }
   
   /**
    * If the right sensor is activated, print a debug message and play the previous
    * track in the playlist using the TMRpcm library.
    */
   if (  digitalRead(rightSensor) == LOW ){
      Serial.println("Right Sensor Activated");
      playPreviousTrack(audio);
   }
    
   /**
    * Get the current distance to an obstacle and update the volume of the audio playback
    * accordingly, using the TMRpcm library.
    */
   int distance = getDistance();
   updateVolumeOnDistanceChange(distance, audio);
   delay(1000);
} 


/**
 * Scans the SD card for music files and adds them to the provided Music object.
 *
 * @param music The Music object to add the music files to.
 */
void getMusics(Music& music ) {

  // Open the root directory of the SD card
  File directory = SD.open("/");

  // Loop through all files and directories in the root directory
  while (true) {

    // Open the next file or directory in the root directory
    File entry =  directory.openNextFile();

    // If there are no more files or directories, exit the loop
    if ( !entry )
        break;

    // If the entry is a directory, skip it and move on to the next entry
    if ( entry.isDirectory() )
        continue;

    // Add the name of the file to the list of music tracks
    music.addMusic( String( entry.name() ) );

    // Close the file
    entry.close();
  }
}


/*
 * Measures the distance between an ultrasonic sensor and an object and returns the result.
 * 
 * @return the distance in centimeters
 */
int getDistance() {
  
  // Send a 10 microsecond pulse to the ultrasonic sensor
  {
    digitalWrite(trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);
  }
  

  // Measure the duration of the pulse
  int time = pulseIn(echo, HIGH);

  /* Calculate the distance based on the speed of sound
   * Distance = time * speed of sound / 2
   * Speed of sound at sea level is 343 meters/second = 0.0343 centimeters/microsecond 
   */
  int distance = (time * 0.0343) / 2.0;

  // Return the distance in centimeters
  return distance;
}


/**
 * Updates the volume of the audio based on the distance of hand from the ultrasonic sensor.
 * 
 * @param distance the distance in centimeters from the ultrasonic sensor
 * @param audio a reference to the TMRpcm object controlling the audio output
 */
void updateVolumeOnDistanceChange(const int distance, const TMRpcm& audio) {
  
  if (distance >= 1 && distance <= 3)
    currentVolume = 1;
  
  else if (distance >= 4 && distance <= 6)
    currentVolume = 2;
  
  else if (distance >= 7 && distance <= 9)
    currentVolume = 3;
    
  else if (distance >= 10 && distance <= 12)
    currentVolume = 4;
  
  else if (distance >= 13 && distance <= 15)
    currentVolume = 5;

  // Set the volume of the audio output
  audio.setVolume(currentVolume);

  // Print the current volume to the serial console
  Serial.println(currentVolume);
}

/**
 * Plays the next music in the playlist using the TMRpcm library and
 * prints debug information about which file is being played.
 *
 * @param audio An object of the TMRpcm class for playing audio files.
 */
void playNextTrack(const TMRpcm& audio){

    /**
     * The name of the music file to play.
     */
    String currentMusicToPlay;
    
    currentMusicToPlay = music.getNextMusic();
    Serial.print("Now Playing: ");
    Serial.print(music.iterator);
    Serial.print(". ");
    Serial.print(currentMusicToPlay);
    Serial.println();

    /**
     * Converts the name of the music file to a C-style string and plays it using
     * the TMRpcm library.
     */
    audio.play( currentMusicToPlay.c_str()  );
}


/**
 * Plays the previous music in the playlist using the TMRpcm library and
 * prints debug information about which file is being played.
 *
 * @param audio An object of the TMRpcm class for playing audio files.
 */
void playPreviousTrack(const TMRpcm& audio){
  
    String currentMusicToPlay;
    
    currentMusicToPlay = music.getPreviousMusic();
    Serial.print("Now Playing: ");
    Serial.print(music.iterator+1);
    Serial.print(". ");
    Serial.print(currentMusicToPlay);
    Serial.println();
    audio.play( currentMusicToPlay.c_str()  );
}


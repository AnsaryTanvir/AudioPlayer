 #pragma once
#include <SPI.h>     // include Arduino SPI library.
#include <SD.h>      // include Arduino SD library.
#include "TMRpcm.h"  // include TMRpcm library.

const int SD_ChipSelectPin = 4;
TMRpcm    audio;


class Music{

  public:
    
    String*  names;
    size_t   size; 
    uint8_t  iterator;
    uint8_t  musicCounter;

    bool     next;
    bool     previous;

    Music(const size_t& size){
      
      this->size    = size ;
      iterator      = 0;
      musicCounter  = 0;
      next       = false;
      previous      = false;
      
      names = new String[size];
    }

    ~Music(){
      delete names;
    }

    String getCurrentMusic(){
      iterator %= musicCounter;
      return names[iterator];
    }

    String getNextMusic(){

      next = true;
      if ( previous )
        iterator++, previous = false;


      if ( iterator == 0 )
          return names[iterator++];
      
      iterator %= musicCounter;
      return names[iterator++];
    }

    String getPreviousMusic(){

      previous = true;
      if ( next )
        iterator--, next = false;

      if ( iterator <= 0 )
          iterator = musicCounter-1;
      else
          iterator--;
          
      return names[iterator];
    }

    static const bool isMusic(const String& string ){
      
      const uint8_t nameSize  = string.length();
      const String  extension = string.substring( nameSize - 4 );
   
      if ( !extension.equalsIgnoreCase(".wav") )
          return false;

      return true;
    }

    const bool addMusic(const String& musicName ){

        if ( !isMusic(musicName) )
            return false;

        if ( musicCounter >= size-1 )
            return false;

        names[musicCounter++] = musicName;
        return true;
    }

    void displayMusics(){
      
      for ( int i = 0; i < musicCounter; i++ ){
          Serial.print(i+1);
          Serial.print(". ");
          Serial.print( names[i]);
          Serial.println("");
      }
      Serial.println("");
    }
};

Music music = Music(8); // Create an instance of music class for atmost 8 music.



void onReceiving(const char& command,const TMRpcm& audio){

    String currentMusicToPlay;
    
    switch( command ){

        case 'L':
            Serial.println("");
            Serial.println("List of Musics: ");
            music.displayMusics();
            Serial.println("");
            break;

        case 'P':
            Serial.println("Audio Paused/Resumed");    
            audio.pause();
            break;
            
        case 'F':
            currentMusicToPlay = music.getNextMusic();
            Serial.print("Now Playing: ");
            Serial.print(music.iterator);
            Serial.print(". ");
            Serial.print(currentMusicToPlay);
            Serial.println();
            audio.play( currentMusicToPlay.c_str()  );
            break;

        case 'B':
            currentMusicToPlay = music.getPreviousMusic();
            Serial.print("Now Playing: ");
            Serial.print(music.iterator+1);
            Serial.print(". ");
            Serial.print(currentMusicToPlay);
            Serial.println();
            audio.play( currentMusicToPlay.c_str()  );
            break;

        default:
          break;
        
    }
}

void getMusics(Music& music ) {

  
  File directory = SD.open("/");
  while (true) {
  
    File entry =  directory.openNextFile();
    if ( !entry )
        break;
      
    if ( entry.isDirectory() )
        continue;

    music.addMusic( String( entry.name() ) );
    entry.close();
  }
}


void setup(void) {
  
  Serial.begin(115200);
  
  Serial.print("Initializing SD card...");
  if ( !SD.begin(SD_ChipSelectPin) ) {
    Serial.println("Failed to initialize SD Card!");
    while(true);
  }
  Serial.println("OK!");
  
  audio.speakerPin = 9;     // Speaker output from digital pin 9.
  audio.setVolume(4);       // 0 - 7 Volume Range , 5 optimal.
  audio.quality(1);         // 1 for 2x oversampling (Recommended), 0 for normal sampling.

  getMusics(music);  
  
}


void loop() {

  if ( Serial.available() ){
    char command = Serial.read();
    onReceiving(command, audio);
  }
 
}

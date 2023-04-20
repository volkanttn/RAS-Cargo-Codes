#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(A2, 4);



void setup() {
  char *commands;
  // put your setup code here, to run once:
int signalLengthLol= receiveSignal(&commands);

for(short int w=0; w<signalLengthLol-1; w++) {
    Serial.println(*(commands+w));
    }
    
    
}

void loop() {
  // put your main code here, to run repeatedly:

}

int * receiveSignal(char** commands){
  const byte address[6] = "ras56"; //this will help us communicate with our module and our module only
char dataChunk[32];

char possiblecommands[7]="fslrpd";
static char commandCodes[216][3];

short int i=0;
short int j=0;
short int l=0;
short int k=0;

short int sizeOfSignal=0;

short int kj=0;

short int pathLength=0;

for(i=0; i<=5; i++){
     for(j=0; j<=5; j++){
      for(k=0; k<=5; k++){
            commandCodes[l][0]=possiblecommands[i];
            commandCodes[l][1]=possiblecommands[j];
            commandCodes[l][2]=possiblecommands[k];
            l++;
    }
    }
    }


Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

while(1){Serial.println("waiting for signal...");


if (radio.available()) {
    radio.read(&sizeOfSignal, sizeof(sizeOfSignal));
    break;
  }
  delay(15);
  }





unsigned char signalReceived[sizeOfSignal];



while(kj<sizeOfSignal-2){
 
while(1){
if (radio.available()) {
  
    radio.read(&dataChunk, sizeof(dataChunk));
    break;delay(5);
  }
  }

for(i=0; i<=30; i++){
  signalReceived[kj]=dataChunk[i];
  
  kj++;
}

}

for(i=0; signalReceived[i]<216 && i<sizeOfSignal-1; i++){
pathLength++;
}

pathLength=pathLength*3;

pathLength=pathLength+sizeOfSignal-i;

char path[pathLength];
*(commands) = new char[pathLength];



kj=0;
for(i=0; signalReceived[i]<216 && i<sizeOfSignal-1; i++){


     path[kj]=commandCodes[signalReceived[i]][0];
     kj++;
     path[kj]=commandCodes[signalReceived[i]][1];
     kj++;
     path[kj]=commandCodes[signalReceived[i]][2];
      kj++;
      
     }


    
for(kj=kj; kj<pathLength-1 ;kj++){
      switch (signalReceived[i]){
        case 216:
          path[kj]='f';
          i++;
          break;

        case 217:
           path[kj]='s';
           i++;
          break;

        case 218:
          path[kj]='l';
           i++;
          break;

        case 219:
          path[kj]='r';
           i++;
          break;

        case 220:
          path[kj]='p';
           i++;
          break;

        case 221:
          path[kj]='d';
           i++;
          break;
          
        case 255:
          path[kj]='e';
           i++;
          break;
      }}

for(i=0; i<pathLength-1; i++) {
    //Serial.print(path[i]);
    }


memcpy(*(commands), path, pathLength);

return pathLength;


}

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(A2, 4);




void setup() {
  // put your setup code here, to run once:

Serial.begin(9600);

char pathToFollow[]="fffsrfsrfslfsprrfsrfslfslfffsrffffsdrrffffslfffsrfslfsrfsprrfslfsrfslfffsrfffsdrrfffslfffsrfsfspllfsfslfffsrffsdrrffslfffsrfsrfslfsprrfsrfslfslfffsrfsdrrfsrddd";
int pathToFollowSize= sizeof(pathToFollow)/sizeof(pathToFollow[0]);


generateSignal(pathToFollow, pathToFollowSize);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void generateSignal(char path[], int pathSize){

Serial.println(path);

char commands[7]="fslrpd";
short int i=0;
short int j=0;
short int k=0;
short int l=0;
char commandCodes[216][3];

const byte address[6] = "ras56"; //this will help us communicate with our module and our module only
unsigned char dataChunk[32];

short int pointOfSingleCommands=0;
short int jk=0;

short int sizeOfPathMinusTwo=pathSize-2;


unsigned char signalToSend[((pathSize-1)-((pathSize-1)%3))/3+((pathSize-1)%3)+1];


  
  //Serial.begin(9600);


l=0;
for(i=0; i<=5; i++){
     for(j=0; j<=5; j++){
      for(k=0; k<=5; k++){
            commandCodes[l][0]=commands[i];
            commandCodes[l][1]=commands[j];
            commandCodes[l][2]=commands[k];
            l++;
    }
    }
    }

pointOfSingleCommands=l;



l=0;

  for(i=0; (i+2)<=sizeOfPathMinusTwo ;i=i+3){
    
     for(j=0; j<=215; j++){
      
      if(commandCodes[j][0]==path[i] && commandCodes[j][1]==path[i+1] && commandCodes[j][2]==path[i+2]){
        signalToSend[l]=j;
        
        
        l++;
      }
     }
    }

    for(jk=0; i<=sizeOfPathMinusTwo ;i++){
      switch (path[i]){
        case 'f':
          signalToSend[l]=pointOfSingleCommands;
          l++; 
          break;

        case 's':
           signalToSend[l]=pointOfSingleCommands+1;
          l++;
          break;

        case 'l':
          signalToSend[l]=pointOfSingleCommands+2;
          l++;
          break;

        case 'r':
          signalToSend[l]=pointOfSingleCommands+3;
          l++;
          break;

        case 'p':
          signalToSend[l]=pointOfSingleCommands+4;
          l++;
          break;

        case 'd':
          signalToSend[l]=pointOfSingleCommands+5;
          l++;
          break;
      }
      
        }
      
    


radio.begin();


l=0;
short int signalMagnitude= (sizeof(signalToSend)/(sizeof(signalToSend[0])))-2;

radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN); //set the power level depending on how far away the modules are
  radio.stopListening();


  short int sigMagPTwo=signalMagnitude+2;
  
radio.write(&sigMagPTwo, sizeof(sigMagPTwo));
//Serial.println(sigMagPTwo);
delay(10);




for(jk=0; jk<=signalMagnitude ;jk=jk+32){
  for(j=0; j<32 ;j++){
  if(l<=signalMagnitude){
    dataChunk[j]=signalToSend[l];
  
  l++;}
  else{
    dataChunk[j]= 255;
    l++;
  }
  
  }
  
  radio.write(&dataChunk, sizeof(dataChunk));
  
  delay(30);
  
  
  }

  



}

char path[]="fffsrfsrfslfsprrfsrfslfslfffsrffffsdrrffffslfffsrfslfsrfsprrfslfsrfslfffsrfffsdrrfffslfffsrfsfspllfsfslfffsrffsdrrffslfffsrfsrfslfsprrfsrfslfslfffsrfsdrrfsr";
char commands[7]="fslrpd";
short int i=0;
short int j=0;
short int k=0;
short int l=0;
char commandCodes[216][3];

short int pointOfSingleCommands=0;
short int jk=0;

short int sizeOfPathMinusTwo=sizeof(path)/(sizeof(path[0]))-2;


unsigned char signalToSend[((sizeof(path)/(sizeof(path[0]))-1)-((sizeof(path)/(sizeof(path[0]))-1)%3))/3+((sizeof(path)/(sizeof(path[0]))-1)%3)+1];

void setup() {
  
  Serial.begin(9600);


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
      
    delay(900);




}

void loop() {
  delay(50);
  
  
  
  
  }

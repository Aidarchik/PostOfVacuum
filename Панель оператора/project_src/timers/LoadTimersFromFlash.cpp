WORD i=0;
azotTime.totalSeconds = MAKEDWORD(GetPFW(506), GetPFW(507));

for(i=0; i<countFixtures; i++){
	process[i].vakAzotTimer.setpoint.totalSeconds = MAKEDWORD(GetPFW(306+i*10), GetPFW(307+i*10));
    process[i].heatingTimer.setpoint.totalSeconds = MAKEDWORD(GetPFW(706+i*10), GetPFW(707+i*10));
    
	if(process[i].heatingTimer.setpoint.totalSeconds > process[i].vakAzotTimer.setpoint.totalSeconds){
    process[i].heatingTimer.setpoint.totalSeconds = process[i].vakAzotTimer.setpoint.totalSeconds;
  }

  //----Decompose time setpoint----
  DecomposeTime(&process[i].vakAzotTimer.setpoint);
  DecomposeTime(&process[i].heatingTimer.setpoint);
  
  if(azotTime.totalSeconds > process[i].vakAzotTimer.setpoint.totalSeconds){
    azotTime.totalSeconds = process[i].vakAzotTimer.setpoint.totalSeconds; 
  }
  
  //---Reset timers---
  ResetTimer(&process[i].vakAzotTimer);
  ResetTimer(&process[i].heatingTimer);
}
DecomposeTime(&azotTime);

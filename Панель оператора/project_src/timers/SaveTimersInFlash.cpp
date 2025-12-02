WORD i=0;
for(i=0; i<countFixtures; i++){
    if(!GetPSBStatus(308+i*10)){
        
		SetPFW(306+i*10, LOWORD(process[i].vakAzotTimer.setpoint.totalSeconds));
		SetPFW(307+i*10, HIWORD(process[i].vakAzotTimer.setpoint.totalSeconds));
        
		SetPFW(706+i*10, LOWORD(process[i].heatingTimer.setpoint.totalSeconds));
		SetPFW(707+i*10, HIWORD(process[i].heatingTimer.setpoint.totalSeconds));
        
    }
}

SetPFW(506, LOWORD(azotTime.totalSeconds));
SetPFW(507, HIWORD(azotTime.totalSeconds));
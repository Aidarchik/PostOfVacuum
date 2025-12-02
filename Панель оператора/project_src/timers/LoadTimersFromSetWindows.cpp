WORD i=0;
azotTime.hour = PSW[503];
azotTime.min = PSW[504];
azotTime.sec = PSW[505];
ComposeTime(&azotTime);

for(i=0; i<countFixtures; i++){
    if(!GetPSBStatus(308+i*10)){
		process[i].vakAzotTimer.setpoint.hour = PSW[303+i*10];
        process[i].vakAzotTimer.setpoint.min = PSW[304+i*10];
        process[i].vakAzotTimer.setpoint.sec = PSW[305+i*10];

		process[i].heatingTimer.setpoint.hour = PSW[703+i*10];
        process[i].heatingTimer.setpoint.min = PSW[704+i*10];
        process[i].heatingTimer.setpoint.sec = PSW[705+i*10];
        
        ComposeTime(&process[i].vakAzotTimer.setpoint);
		ComposeTime(&process[i].heatingTimer.setpoint);
    
		if(process[i].heatingTimer.setpoint.totalSeconds > process[i].vakAzotTimer.setpoint.totalSeconds){
			process[i].heatingTimer.setpoint.totalSeconds = process[i].vakAzotTimer.setpoint.totalSeconds;
			//---Decompose time setpoint---//
			DecomposeTime(&process[i].vakAzotTimer.setpoint);
			DecomposeTime(&process[i].heatingTimer.setpoint);
		}

		if(azotTime.totalSeconds > process[i].vakAzotTimer.setpoint.totalSeconds){
			azotTime.totalSeconds = process[i].vakAzotTimer.setpoint.totalSeconds;
			DecomposeTime(&azotTime); 
		}

		//---Reset timers---//
		ResetTimer(&process[i].vakAzotTimer);
		ResetTimer(&process[i].heatingTimer);
    }
    
}
		
WORD i=0;
PSW[503] = azotTime.hour;
PSW[504] = azotTime.min;
PSW[505] = azotTime.sec;

for(i=0; i<countFixtures; i++){
    if(!GetPSBStatus(308+i*10)){
		PSW[303+i*10] = process[i].vakAzotTimer.setpoint.hour;
		PSW[304+i*10] = process[i].vakAzotTimer.setpoint.min;
		PSW[305+i*10] = process[i].vakAzotTimer.setpoint.sec;
    
		PSW[703+i*10] = process[i].heatingTimer.setpoint.hour;
		PSW[704+i*10] = process[i].heatingTimer.setpoint.min;
		PSW[705+i*10] = process[i].heatingTimer.setpoint.sec;
    }
}
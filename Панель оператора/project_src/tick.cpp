WORD i=0;
toggleBlink();

for(i=0; i<countFixtures; i++){
	
	//---Async Timers---
	if(process[i].main_state == ST_SYNC_WAIT){
		process[i].asyncTimer--;
	}
	
	//---work Timers---
	if(process[i].vakAzotTimer.enableTick){
		TickTimer(&process[i].vakAzotTimer);
	}
	if(process[i].heatingTimer.enableTick){
		TickTimer(&process[i].heatingTimer);
	}
}
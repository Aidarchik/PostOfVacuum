WORD i=0;
//---LOAD DATA---//
//---Set proporties temperature--//
LoadAdminParametrs();
LoadSetpointsFromFlash();
LoadTimersFromFlash();

//---RESET FLAGS---//
ResetPSB(500); // limit vacuum

//---limit azot---//
ResetPSB(601);
SetPSB(602);
ResetPSB(603);

//---Reset valve---//
Write(PLC, 1, MODBUS_RTU_REG_4X, 512, 0, TYPE_WORD, 0x00);

for(i=0; i<countFixtures; i++){
    fixture.values.temperature[i] = 25.0;
    process[i].main_state = ST_IDLE;
    //---Enable Timers---//
	process[i].vakAzotTimer.enableTick = FALSE; 
    process[i].heatingTimer.enableTick = FALSE;
    
    ResetPSB(308+i*10); //  Run/stop process
    SetPSB(408+i*10); //Show setTimer button flag
    
    //---limit temperatures---//
    ResetPSB(701+i*10);
    SetPSB(702+i*10);
    ResetPSB(703+i*10);
}

//---SHOW DATA---//
ShowSetpointsOnDisplay();
ShowTimersOnDisplay();

//---SET POSITION WINDOWS OF Done---//
Done[0].winID=7; Done[0].winX=13; Done[0].winY=116;
Done[1].winID=8; Done[1].winX=275; Done[1].winY=116;
Done[2].winID=9; Done[2].winX=537; Done[2].winY=116;
Done[3].winID=10; Done[3].winX=13; Done[3].winY=298;
Done[4].winID=11; Done[4].winX=275; Done[4].winY=298;
Done[5].winID=12; Done[5].winX=537; Done[5].winY=298;
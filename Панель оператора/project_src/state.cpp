WORD i=0;
CheckLimits(&fixture);

//---Set/reset bit of state limits vacuum---//
ResetPSB(501);
ResetPSB(502);
if(blink && (fixture.limits.vacuum.state == HIGH)) SetPSB(501);
if(fixture.limits.vacuum.state == NORM) SetPSB(502);

//---Set/reset bits of state limits azot---// 
ResetPSB(601);
ResetPSB(602);
ResetPSB(603);
if(blink && (fixture.limits.azot.state == HIGH)) SetPSB(601);
if(fixture.limits.azot.state == NORM) SetPSB(602);
if(blink && (fixture.limits.azot.state == LOW)) SetPSB(603);

//---Set/reset bits of state limits Temperature---// 
for(i=0; i<countFixtures; i++){
    if(GetPSBStatus(308+i*10)) {
        ResetPSB(701+i*10); // HIGH
        ResetPSB(702+i*10); // NORM
        ResetPSB(703+i*10); // LOW
        if(blink && (fixture.limits.temperature[i].state == HIGH)) SetPSB(701+i*10);
        if(fixture.limits.temperature[i].state == NORM) SetPSB(702+i*10);
        if(blink && (fixture.limits.temperature[i].state == LOW)) SetPSB(703+i*10);
    }
    else{
        //---Reset flag temperature---//
        ResetPSB(701+i*10);
        SetPSB(702+i*10);
        ResetPSB(703+i*10);
    }
}

//---Start/stop/reset process---//
for(i=0; i<countFixtures; i++){

    //---Start process and hide setTimer Buttons---//
    if(GetPSBStatus(308+i*10) && GetPSBStatus(902)) {
        process[i].start_signal = TRUE; //Ready process to start
        //Show text "Process..."
        if(blink) SetPSB(408+i*10);
        else ResetPSB(408+i*10); 
    }
    if(!GetPSBStatus(902)){
        process[i].stop_signal = TRUE; //Ready process to stop
        ResetPSB(408+i*10); //hide text "Process..."
    }
    //---Stop process and hide text "Process..."---//
    if(process[i].main_state == ST_DONE){
        ResetPSB(308+i*10);
        ResetPSB(408+i*10); //hide text "Process..."
        process[i].main_state = ST_IDLE;
        OpenWindow(Done[i].winID, Done[i].winX, Done[i].winY);
    }
}

//---check of stoped all process---//
fixture.stopedAllProcesses = TRUE;
for(i=0; i<countFixtures; i++){
    if(GetPSBStatus(308+i*10)) fixture.stopedAllProcesses = FALSE;
}
if(fixture.stopedAllProcesses) ResetPSB(902); 


updateFixtureProcess();

//---check temperature Overload---//
for(i=0; i<countFixtures; i++){
    ResetPSB(705+i*10);
    if(fixture.temperatureOverload[i]) SetPSB(705+i*10);
}
//---check heaterFail---//
for(i=0; i<countFixtures; i++){
    ResetPSB(704+i*10);
    if(process[i].heaterFail) SetPSB(704+i*10);
}

for(i=0; i<countFixtures; i++){
    //---show current time---//
    PSW[403+i*10] = process[i].vakAzotTimer.current.hour;
    PSW[404+i*10] = process[i].vakAzotTimer.current.min;
    PSW[405+i*10] = process[i].vakAzotTimer.current.sec;
    
    PSW[803+i*10] = process[i].heatingTimer.current.hour;
    PSW[804+i*10] = process[i].heatingTimer.current.min;
    PSW[805+i*10] = process[i].heatingTimer.current.sec;
}

//---Hide/show back button and check of ready all process---//

if(fixture.launchingProcesses) ResetPSB(901);
else {
    SetPSB(901);
}




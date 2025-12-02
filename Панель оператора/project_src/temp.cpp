#define countFixtures 6
BOOL blink;
WORD i=0;
WORD wTemp=0;
WORD valvesState=0;
WORD oldValvesState=0;

void toggleBlink(){blink=!blink;}

WORD GetPFW(WORD Adress){
	Read(HMI_LOCAL_MCH, 0, TYPE_PFW, Adress, 0, TYPE_WORD, &wTemp); 
	return wTemp;
}
void SetPFW(WORD Adress, WORD Value){
	Write(HMI_LOCAL_MCH, 0, TYPE_PFW, Adress, 0, TYPE_WORD, Value);
}

WORD GetLoWORD(float FloatValue) {return LOWORD(Float_2_DWord(FloatValue));}
WORD GetHiWORD(float FloatValue) {return HIWORD(Float_2_DWord(FloatValue));}

//---Done---//
typedef struct{
    WORD winID;
    WORD winX;
    WORD winY;
} WinDone;

WinDone Done[countFixtures];

//---------------------//
//------Values---------//
//---------------------//
typedef enum {
	HIGH,
    NORM,
    LOW,
} LimitState;

typedef struct{
	WORD minmax;
	LimitState state;
    BOOL newValveStarted;
    BOOL checkAllValveIsOpened;
} ParamLimits;

typedef struct{
	float vacuum;
	WORD azot;
	WORD temperature[countFixtures];
} FixtureParams;

typedef struct{
    ParamLimits vacuum;
	ParamLimits azot;
	ParamLimits temperature[countFixtures];
} FixtureLimits;

typedef struct{
    FixtureParams values;
	FixtureParams setpoints;
	FixtureLimits limits;
} FixtureState;

FixtureState fixture;

void ResetFixtureLimits(FixtureState* f){
    WORD i=0;
    f->limits.vacuum.state = NORM; //---reset state vacuum---//
	f->limits.azot.state = NORM; //---reset state azot---//
    for(i=0; i<countFixtures; i++){
         f->limits.temperature[i].state = NORM; //---reset state temperatures---//
	}
}


void CheckLimits(FixtureState* f){
    WORD i=0;
	//---check limit vakuum---//
    if((f->values.vacuum) > (f->setpoints.vacuum)){
        (f->limits.vacuum.state) = HIGH;
    }
    else (f->limits.vacuum.state) = NORM;
    
    //---check limit azot---//
    if((f->values.azot - f->setpoints.azot) > (f->limits.azot.minmax)){
        (f->limits.azot.state) = HIGH;
    }
	else{
        if(((f->setpoints.azot) - (f->values.azot)) > (f->limits.azot.minmax)){
            (f->limits.azot.state) = LOW;
        }
			else f->limits.azot.state = NORM;
    }

    //---check limit temperatures---//
    for(i=0; i<countFixtures; i++){
        if((f->values.temperature[i] - f->setpoints.temperature[i]) > (f->limits.temperature[i].minmax)) {
            f->limits.temperature[i].state = HIGH;
        }
		else{
            if((f->setpoints.temperature[i] - f->values.temperature[i]) > (f->limits.temperature[i].minmax)) {
                f->limits.temperature[i].state = LOW;
            }
            else f->limits.temperature[i].state = NORM;
        }
            
    }
}



void UpdateSetpoints(){
    WORD i=0;
	//---updateSetPoint for Vacuum---
	DWORD temp=MAKEDWORD(PSW[508],PSW[509]);
	fixture.setpoints.vacuum=DWord_2_Float(temp);
    
	//---updateSetPoint for Azot---
	fixture.setpoints.azot=PSW[608];
	fixture.limits.azot.minmax=PSW[609]; //Limit
    
	//---updateSetPoint for Temperature---
	for(i=0; i<countFixtures; i++){
        fixture.setpoints.temperature[i]=PSW[708+i*10];
        fixture.limits.temperature[i].minmax=PSW[709+i*10]; //Limit
	}
    
}

//---------------------//
//------Timers---------//
//---------------------//
typedef struct{
    DWORD totalSeconds;
    WORD hour;
    WORD min;
    WORD sec;
} TimeFormatted;

typedef struct{
    TimeFormatted setpoint;
	TimeFormatted current;
    BOOL enableTick;
} TimerData;

void ComposeTime(TimeFormatted* t){
    t->totalSeconds = t->hour*60*60+t->min*60+t->sec;
}


void DecomposeTime(TimeFormatted* t){
	t->hour = t->totalSeconds/3600;
	t->min = t->totalSeconds%3600/60;	
	t->sec = t->totalSeconds%3600%60;
}

void TickTimer(TimerData* t){
    if((t->current.totalSeconds) > 0){
        t->current.totalSeconds--;
		DecomposeTime(&t->current);
    }
}

void ResetTimer(TimerData* t){
    t->current.totalSeconds = t->setpoint.totalSeconds;
    DecomposeTime(&t->current);
}

//-------------//
//---Process---//
//-------------//
typedef enum{
    EVT_NONE,
    EVT_START,
    EVT_VACUUM_LIMIT,
    EVT_AZOT_LIMIT,
    EVT_LAST_OPENED_VACUUM_VALVE,
    EVT_CHECK_LAST_OPENED_VACUUM_VALVE,
    EVT_ERROR_VACUUM_VALVE,
    EVT_LAST_OPENED_AZOT_VALVE,
    EVT_CHECK_LAST_OPENED_AZOT_VALVE
} EventType;

typedef struct{
    EventType type;
    WORD target_id;
} Event;

Event event;

typedef enum {
    ST_IDLE,
    ST_VACUUM_VALVE_OPEN,
    ST_SYNC_WAIT,
    ST_VACUUM_AZOT,
    ST_DONE,
    ST_STOP,
    ST_LIMIT_AZOT
} FixtureMainState;

typedef enum{
    VA_IDLE,
    VA_VACUUM_ON,
    VA_LIMIT_VACUUM,
    VA_WAIT_VALVE_AFTER_OFF,
    VA_AZOT_ON,
    VA_FINISHED
} VASubState;

typedef struct{
	BOOL open;
	BOOL was_open;
	BOOL was_open_before_fixture_limits;
} Valve;

typedef struct{
    BOOL new_fixture_started;
    BOOL start_requested;
    BOOL stop_requested;
} FixtureEvents;

typedef struct{
    FixtureMainState main_state;
    FixtureMainState prev_main_state;
    VASubState va_state;
    VASubState prev_va_state;
    
    //---Outputs---//
    Valve vacuum_valve;
    Valve azot_valve;
    BOOL heater;
    
	//---Timers---//
	TimerData vakAzotTimer;
	TimerData heatingTimer;

    FixtureEvents events;
    BOOL start_signal;
    BOOL prev_start_signal;
    BOOL stop_signal;
    BOOL prev_stop_signal;
} FixtureProcess;

FixtureProcess process[countFixtures];
TimeFormatted azotTime;

void GetValuesFromDevice(){
    WORD i=0;
	fixture.values.vacuum=0.02;
    if(process[4].vacuum_valve.open){
        fixture.values.vacuum=0.03;
    }
    fixture.values.azot=760;
    for(i=0; i<countFixtures; i++){
        fixture.values.temperature[i]=65;
    }
}

DWORD getLeftTime(TimerData* t){
    return ((t->setpoint.totalSeconds)-((t->setpoint.totalSeconds)-(t->current.totalSeconds)));
}

void processStartOrStopCommands(){
    BOOL start_now;
    BOOL stop_now;
    WORD i=0;
    for(i=0; i<countFixtures; i++){
        //---click start button---//
        start_now = process[i].start_signal && !process[i].prev_start_signal;
        process[i].prev_start_signal = process[i].start_signal;
        process[i].start_signal = FALSE;
        if(start_now) process[i].events.start_requested = TRUE;

        //---click stop button---//
        stop_now = process[i].stop_signal && !process[i].prev_stop_signal;
        process[i].prev_stop_signal = process[i].stop_signal;
        process[i].stop_signal = FALSE;
        if(stop_now) process[i].main_state = ST_STOP;
    }
}

void closeAllOtherValves(WORD index){
    WORD i=0;
    for(i=0; i<countFixtures; i++){
        if(i != index){
            if(process[i].vacuum_valve.open && !process[i].events.start_requested){
                process[i].vacuum_valve.was_open = TRUE;
                process[i].vacuum_valve.open=FALSE;
            }
        }
    }
}

void restoreAllOtherVacuumValveState(WORD index){
    WORD i=0;
    //---fixture started before wait---//
    for(i=0; i<countFixtures; i++){
        if(i == (countFixtures-1)) {
            process[index].events.new_fixture_started=FALSE;
            fixture.limits.vacuum.newValveStarted = FALSE;
        }
        if(i != index){
            if (process[i].vacuum_valve.was_open){
                process[i].vacuum_valve.was_open = FALSE;
                process[i].vacuum_valve.open=TRUE;
            }
        }
    }
}
void updatePressureOK(){
    WORD i=0;

    if(fixture.limits.vacuum.state==NORM){
        //---check the event of the EVT_LAST_OPENED_VACUUM_VALVE for leaks---//
        if(event.type == EVT_CHECK_LAST_OPENED_VACUUM_VALVE){
            event.type = EVT_NONE;
        }
        if(event.type == EVT_LAST_OPENED_VACUUM_VALVE){
            if (process[event.target_id].vacuum_valve.was_open){
                process[event.target_id].vacuum_valve.was_open = FALSE;
                process[event.target_id].vacuum_valve.open=TRUE;
                event.type=EVT_CHECK_LAST_OPENED_VACUUM_VALVE;
            }
        }
        else if(!fixture.limits.vacuum.newValveStarted){
            for(i=0; i<countFixtures; i++){
                if (process[i].vacuum_valve.was_open){
                    process[i].vacuum_valve.was_open = FALSE;
                    process[i].vacuum_valve.open=TRUE;
                }
            }
        }
    }


    if(fixture.limits.vacuum.state!=NORM){
        if(event.type==EVT_CHECK_LAST_OPENED_VACUUM_VALVE){
            event.type=EVT_ERROR_VACUUM_VALVE;
            process[event.target_id].main_state=ST_STOP;
        }
        for(i=0; i<countFixtures; i++){
            if(process[i].vacuum_valve.open && !process[i].events.start_requested){
                process[i].vacuum_valve.was_open = TRUE;
                process[i].vacuum_valve.open=FALSE;
            }
        }
    }
}

BOOL updateOutputs(WORD index){
    BOOL waitOpen;
    waitOpen = FALSE;
    //---SET RESET OUTPUTS---//
    if(process[index].vacuum_valve.open){
        valvesState = valvesState | (1<<index);
        if(oldValvesState != valvesState){
            waitOpen = TRUE;
            if(event.type != EVT_CHECK_LAST_OPENED_VACUUM_VALVE){
                event.type = EVT_LAST_OPENED_VACUUM_VALVE;
                event.target_id = index;
            }
        }
    } 
    if(process[index].azot_valve.open){
        valvesState = valvesState | (1<<(index+6));
        if(oldValvesState != valvesState){
            waitOpen = TRUE;
            event.type = EVT_LAST_OPENED_AZOT_VALVE;
            event.target_id = index;
        }
    }
    // if(process[index].heater) SetPSB(708+index*10);
    if(!process[index].vacuum_valve.open) valvesState = valvesState &~ (1<<index);
    if(!process[index].azot_valve.open) valvesState = valvesState &~ (1<<(index+6));
    // if(!process[index].heater) ResetPSB(708+index*10);

    if((oldValvesState != valvesState) && waitOpen){
        Write(PLC, 1, MODBUS_RTU_REG_4X, 512, 0, TYPE_WORD, valvesState);
        oldValvesState = valvesState;
    }
    return waitOpen;
}
void fixtureProcess(WORD index, FixtureProcess* process, FixtureState* fixture, TimeFormatted* azotTime){ 
    switch(process->main_state){
        case ST_IDLE:{
            if(process->events.start_requested){
                closeAllOtherValves(index);
                process->vacuum_valve.was_open=FALSE;
                process->vacuum_valve.open=FALSE;
                process->main_state=ST_VACUUM_VALVE_OPEN;
            }
			else{
                process->vacuum_valve.was_open = FALSE;
				process->vacuum_valve.open=FALSE;
				process->azot_valve.open=FALSE;
				process->heater=FALSE;
			}
            break;
        }
        case ST_VACUUM_VALVE_OPEN:{
            process->vacuum_valve.open=TRUE;
            fixture->limits.vacuum.newValveStarted = TRUE;
            process->main_state=ST_SYNC_WAIT;
            break;
        }
        case ST_SYNC_WAIT:{
            if((fixture->limits.vacuum.state == NORM) && (fixture->limits.azot.state == NORM)){
                process->events.start_requested=FALSE;
                process->main_state=ST_VACUUM_AZOT;
                process->va_state=VA_VACUUM_ON;
                process->heater=TRUE;
                process->vakAzotTimer.enableTick=TRUE;
                process->heatingTimer.enableTick=TRUE;
                process->events.new_fixture_started=TRUE;
            }
            break;
        }
        case ST_VACUUM_AZOT:{
            switch(process->va_state){
                case VA_VACUUM_ON:{
                    if(process->events.new_fixture_started){
                        restoreAllOtherVacuumValveState(index);
                    }
                    if(getLeftTime(&process->vakAzotTimer) <= (azotTime->totalSeconds)){
                        process->vacuum_valve.was_open = FALSE;
                        process->vacuum_valve.open=FALSE;
                        process->va_state=VA_WAIT_VALVE_AFTER_OFF;
                    }
                    break;
                }
                case VA_WAIT_VALVE_AFTER_OFF:{
                    process->azot_valve.open=TRUE;
                    process->va_state=VA_AZOT_ON;
                    break;
                }
                case VA_AZOT_ON:{
                    if(fixture->limits.azot.state != NORM){
                        process->azot_valve.open=FALSE;
                    }
                    else{
                        process->azot_valve.open=TRUE;
                    }
                    if(getLeftTime(&process->vakAzotTimer) <= 0){
                        process->vakAzotTimer.enableTick=FALSE;
                        process->azot_valve.open=FALSE;
                        ResetTimer(&process->vakAzotTimer);
                        ResetTimer(&process->heatingTimer);
                        process->va_state=VA_FINISHED;
                        process->main_state=ST_DONE;       
                    }
                    break;
                    }
				default:
                break;
            }
            if(getLeftTime(&process->heatingTimer) <= 0){
                process->heatingTimer.enableTick=FALSE;
                process->heater=FALSE;
            }
            break;
        }
        case ST_DONE:{
            break;
        }
        case ST_STOP:{
            process->vacuum_valve.was_open = FALSE;
            process->vacuum_valve.open=FALSE;
            process->azot_valve.open=FALSE;
            process->heater=FALSE;
            process->vakAzotTimer.enableTick=FALSE;
            process->heatingTimer.enableTick=FALSE;
            ResetTimer(&process->vakAzotTimer);
            ResetTimer(&process->heatingTimer);
            process->va_state=VA_FINISHED;
            process->main_state=ST_IDLE;       
            break;
        }
    }
            


}


void updateFixtureProcess(){
    WORD i=0;
    BOOL closeValves;
    processStartOrStopCommands();
    updatePressureOK();
    for(i=0; i<countFixtures; i++){
        fixtureProcess(i, &process[i], &fixture, &azotTime);
    }
    for(i=0; i<countFixtures; i++){
        if(updateOutputs(i)){
            Delay(300);
            break;
        }
        if(i == (countFixtures-1)){
            if(oldValvesState != valvesState){
                Write(PLC, 1, MODBUS_RTU_REG_4X, 512, 0, TYPE_WORD, valvesState);
                Delay(600);
                oldValvesState = valvesState;
            }
        }
    }
}


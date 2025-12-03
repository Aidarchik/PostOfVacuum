#define countFixtures 6
BOOL blink;
// WORD i=0;
WORD wTemp=0;
WORD valvesState=0;
WORD oldValvesState=0;
WORD heatersState=0;
WORD oldHeatersState=0;
float T_amb; //---temperatura v kabinete
float K; //---Koeficient usilenia
float alpha; //---Inercia

//---For convert temperature Fixture/Heater---//

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
    BOOL testLeak;
    WORD indexOfLastClosedValve;
} ParamLimits;

typedef struct{
	float vacuum;
	float azot;
	float temperature[countFixtures];
	float temperature_offset[countFixtures];
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
    BOOL launchingProcesses;
    BOOL stopedAllProcesses;
    BOOL temperatureOverload[countFixtures];
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

float abs_x;
short exponent;
float mantissa;

float my_fabs(float x){
    if(x<0) return -x;
    else return x;
}

short word_log10(float x){
    WORD exponent = 0;
    if (x >= 1.0){
        while (x >= 10.0){
            x /= 10.0;
            exponent++;
        }
    } else if (x > 0.0){
        while (x < 1.0){
            x*=10.0;
            exponent--;
        }
    }
    return exponent;
}

float pow10(short n){
    float res;
    short i;
    res = 1.0;
    if(n >= 0){
        for(i=0; i < n; i++){
            res*=10; 
        }
    } else{
        for(i=0; i<-n; i++){
            res/=10;
        }
    }
    return res;
}

void UpdateSetpoints(){
    WORD i=0;
	//---updateSetPoint for Vacuum---
	DWORD temp=MAKEDWORD(PSW[508],PSW[509]);
    mantissa = DWord_2_Float(temp);
    exponent = Word_2_Int16(PSW[510]);
    fixture.setpoints.vacuum=mantissa*pow10(exponent);

    //---updateSetPoint for Azot---
    temp=MAKEDWORD(PSW[608],PSW[609]);
	fixture.setpoints.azot=DWord_2_Float(temp);
	fixture.limits.azot.minmax=PSW[610]; //Limit
    
	//---updateSetPoint for Temperature---
	for(i=0; i<countFixtures; i++){
        temp=MAKEDWORD(PSW[708+i*10],PSW[709+i*10]);
        fixture.setpoints.temperature[i]=DWord_2_Float(temp);
        fixture.limits.temperature[i].minmax=PSW[702+i*10]; //Limit
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
typedef enum {
    ST_IDLE,
    ST_VACUUM_VALVE_OPEN,
    ST_SYNC_WAIT,
    ST_START_TICK,
    ST_VACUUM_AZOT,
    ST_DONE,
    ST_STOP,
    ST_LIMIT_VA,
    ST_ERROR,
    ST_HEATER_FAIL
} FixtureMainState;

typedef enum{
    VA_VACUUM_ON,
    VA_LIMIT_VACUUM,
    VA_WAIT_VALVE_AFTER_OFF,
    VA_AZOT_ON,
    VA_FINISHED
} VASubState;

typedef struct{
	BOOL open;
	BOOL was_open;
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
    BOOL heaterFail;
    
	//---Timers---//
    WORD asyncTimer;
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


//---Correctirovka temperaturi izdelia otnositelno temperaturi nagrevatelia---//
void TemperatureHeater2Fixture(float* T_fixture, float* T_heater, float T_amb, float K, float alpha){
    float T_ss = T_amb + K * (*T_heater - T_amb);
    *T_fixture += alpha * (T_ss - *T_fixture);
}

//---Obratnaia funktcia dlya polucheniya temperaturi nagrevatelya otnositelno temperaturi izdelia---//
void TemperatureFixture2Heater(float* T_fixture_target, float* T_heater, float T_amb, float K){
    *T_heater = T_amb + (*T_fixture_target-T_amb)/K;
}

void GetValuesFromDevice(){
    WORD i=0;
    WORD ArrValue[2];
    WORD ArrValueTemperature[12];
    WORD isError[6];
    DWORD Arr_2_DW;
    float T_heater;

    //---Get vacuum values---//
    Reads(PLC, 2, MODBUS_RTU_REG_4X, 0, 2, &ArrValue);
    Arr_2_DW = MAKEDWORD(ArrValue[0], ArrValue[1]);
    if (ArrValue[1] >= 65408){
        SetPSB(504);
        fixture.values.vacuum = 0.2;
    }
    else {
        ResetPSB(504);
        fixture.values.vacuum=DWord_2_Float(Arr_2_DW);
    }
    
    //---Get azot values---//
    Reads(PLC, 1, MODBUS_RTU_REG_4X, 513, 2, &ArrValue);
    Arr_2_DW = MAKEDWORD(ArrValue[0], ArrValue[1]);
    fixture.values.azot=DWord_2_Float(Arr_2_DW)*7.501;

    //---Get temperatures---//
    Reads(PLC, 16, MODBUS_RTU_REG_4X, 0, 12, &ArrValueTemperature);
    for(i=0; i<countFixtures; i++){
        Arr_2_DW = MAKEDWORD(ArrValueTemperature[i*2+1], ArrValueTemperature[i*2]);
        T_heater = DWord_2_Float(Arr_2_DW);
        TemperatureHeater2Fixture(&fixture.values.temperature[i], &T_heater, T_amb, K, alpha);
        if((fixture.values.temperature[i] > 1000.0) || (fixture.values.temperature[i] < -1000.0)){
            fixture.temperatureOverload[i] = TRUE;
            fixture.values.temperature[i] = 0.0;
        } 
        else{
            fixture.temperatureOverload[i] = FALSE;
        }
    }
    //---Check heaterFail---//
    Reads(PLC, 16, MODBUS_RTU_REG_4X, 96, 6, &isError);
    for(i=0; i<countFixtures; i++){
        if(isError[i] == 2) {
            process[i].heaterFail = TRUE;
            if(process[i].main_state != ST_IDLE){
                process[i].prev_main_state= process[i].main_state;
                process[i].main_state = ST_HEATER_FAIL;
                process[i].vakAzotTimer.enableTick=FALSE;
                process[i].heatingTimer.enableTick=FALSE;
            }
        }
        else process[i].heaterFail = FALSE; 
    }
}

SetSetpointsTemperatureOnDevice(){
    WORD i=0;
    float temperatureFixture2Heater;
    WORD ArrValue[2];

    //---show setpoint Temperature---
    for(i=0; i<countFixtures; i++){
        TemperatureFixture2Heater(&fixture.setpoints.temperature[i], &temperatureFixture2Heater, T_amb, K);
        ArrValue[0]=GetHiWORD(temperatureFixture2Heater);
        ArrValue[1]=GetLoWORD(temperatureFixture2Heater);
        Writes(PLC, 16, MODBUS_RTU_REG_4X, 1872+i*6, 2, ArrValue);
    }
}

DWORD getLeftTime(TimerData* t){
    return ((t->setpoint.totalSeconds)-((t->setpoint.totalSeconds)-(t->current.totalSeconds)));
}

void processStartOrStopCommands(){
    BOOL start_now;
    BOOL stop_now;
    WORD i=0;

    fixture.launchingProcesses = FALSE; 
    for(i=0; i<countFixtures; i++){
        if(process[i].main_state != ST_IDLE) fixture.launchingProcesses = TRUE;

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

void closeAllOtherValves(){
    WORD i=0;
    for(i=0; i<countFixtures; i++){
        if(process[i].vacuum_valve.open){
            process[i].vacuum_valve.was_open = TRUE;
            process[i].vacuum_valve.open=FALSE;
        }
    }
}

void restoreAllOtherVacuumValveState(){
    WORD i=0;
    for(i=0; i<countFixtures; i++){
        if (process[i].vacuum_valve.was_open){
            process[i].vacuum_valve.was_open = FALSE;
            process[i].vacuum_valve.open=TRUE;
        }
    }
}

void updateOutputs(){
    WORD i=0;
    WORD j=0;
    for(i=0; i<countFixtures; i++){

        //---SET RESET OUTPUTS---//
        if(process[i].vacuum_valve.open) valvesState = valvesState | (1<<i);
        if(process[i].azot_valve.open) valvesState = valvesState | (1<<(i+6));
        if(process[i].heater) heatersState = heatersState | (1<<i);
        if(!process[i].vacuum_valve.open) valvesState = valvesState &~ (1<<i);
        if(!process[i].azot_valve.open) valvesState = valvesState &~ (1<<(i+6));
        if(!process[i].heater) heatersState = heatersState &~ (1<<i);

        if(i == (countFixtures-1)){
            if(oldHeatersState != heatersState){
                for(j=0; j<countFixtures; j++){
                    if(heatersState & (1<<j)) {
                        Write(PLC, 16, MODBUS_RTU_REG_4X, 96+j, 0, TYPE_WORD, 0x00);
                    }
                    else{
                        Write(PLC, 16, MODBUS_RTU_REG_4X, 96+j, 0, TYPE_WORD, 0x01);
                    }
                }
                oldHeatersState = heatersState;
            }
            if(oldValvesState != valvesState){
                Write(PLC, 1, MODBUS_RTU_REG_4X, 512, 0, TYPE_WORD, valvesState);
                Delay(600);
                oldValvesState = valvesState;
            }
        }
    }
}



void fixtureProcess(WORD index, FixtureProcess* process){ 
    switch(process->main_state){
        case ST_IDLE:{
            if(process->events.start_requested){
                // closeAllOtherValves();
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
            process->asyncTimer = 5; // 3 sec to macros "tick"
            process->main_state=ST_SYNC_WAIT;
            break;
        }
        case ST_SYNC_WAIT:{
            if(process->asyncTimer <= 0){
                process->main_state=ST_START_TICK;
            }
            break;
        }
        case ST_START_TICK:{
            if(fixture.limits.vacuum.state==NORM){
                process->events.start_requested=FALSE;
                process->main_state=ST_VACUUM_AZOT;
                process->va_state=VA_VACUUM_ON;
                process->heater=TRUE;
                process->vakAzotTimer.enableTick=TRUE;
                process->heatingTimer.enableTick=TRUE;
            }
            break;
        }
        case ST_VACUUM_AZOT:{
            switch(process->va_state){
                case VA_VACUUM_ON:{
                    if(getLeftTime(&process->vakAzotTimer) <= (azotTime.totalSeconds)){
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
            process->events.start_requested=FALSE;
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
        case ST_HEATER_FAIL:{
            if(!process->heaterFail) {
                process->main_state= process->prev_main_state;
                process->vakAzotTimer.enableTick=TRUE;
                process->heatingTimer.enableTick=TRUE;
            }
            break;
        }
    }
            


}


void updateFixtureProcess(){
    WORD i=0;
    processStartOrStopCommands();
    for(i=0; i<countFixtures; i++){
        fixtureProcess(i, &process[i]);
    }
    updateOutputs();
}


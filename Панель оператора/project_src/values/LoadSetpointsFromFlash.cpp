WORD i=0;
//---getSetPoint for Vacuum---
DWORD temp=MAKEDWORD(GetPFW(508),GetPFW(509));
fixture.setpoints.vacuum=DWord_2_Float(temp);
    
//---getSetPoint for Azot---
temp=MAKEDWORD(GetPFW(608),GetPFW(609));
fixture.setpoints.azot=DWord_2_Float(temp);
fixture.limits.azot.minmax=GetPFW(610);

//---getSetPoint for Temperature---
for(i=0; i<countFixtures; i++){
    temp=MAKEDWORD(GetPFW(708+i*10),GetPFW(709+i*10));
	fixture.setpoints.temperature[i]=DWord_2_Float(temp);
    fixture.limits.temperature[i].minmax=GetPFW(702+i*10);
}

SetSetpointsTemperatureOnDevice();


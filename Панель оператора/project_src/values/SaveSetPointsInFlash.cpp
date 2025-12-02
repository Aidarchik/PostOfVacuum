WORD i=0;
UpdateSetpoints();
//---setPoint for Vacuum----
SetPFW(508, GetLoWORD(fixture.setpoints.vacuum));
SetPFW(509, GetHiWORD(fixture.setpoints.vacuum));
	
//---setPoint for Azot
SetPFW(608, GetLoWORD(fixture.setpoints.azot));
SetPFW(609, GetHiWORD(fixture.setpoints.azot));
SetPFW(610, fixture.limits.azot.minmax);

//---setPoint for Temperature
for(i=0; i<countFixtures; i++){
	SetPFW((708+i*10),  GetLoWORD(fixture.setpoints.temperature[i]));
    SetPFW((709+i*10), GetHiWORD(fixture.setpoints.temperature[i]));
    SetPFW((702+i*10), fixture.limits.temperature[i].minmax);
}

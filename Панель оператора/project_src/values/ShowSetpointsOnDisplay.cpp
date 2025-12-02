WORD i=0;
//---show setpoint Vacuum---
abs_x = my_fabs(fixture.setpoints.vacuum);
exponent = word_log10(abs_x);
mantissa = abs_x/pow10(exponent);
if (fixture.setpoints.vacuum < 0){
    mantissa = -mantissa;
}
PSW[510] = exponent;
*(float*)(PSW+508) = mantissa;

//---show setpoint Azot---
PSW[608]=GetLoWORD(fixture.setpoints.azot);
PSW[609]=GetHiWORD(fixture.setpoints.azot);
PSW[610]=fixture.limits.azot.minmax;
    
//---show setpoint Temperature---
for(i=0; i<countFixtures; i++){
	PSW[708+i*10]=GetLoWORD(fixture.setpoints.temperature[i]);
    PSW[709+i*10]=GetHiWORD(fixture.setpoints.temperature[i]);
    PSW[702+i*10]=fixture.limits.temperature[i].minmax;
}

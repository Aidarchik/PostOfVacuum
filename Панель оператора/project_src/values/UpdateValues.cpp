WORD i=0;
GetValuesFromDevice();

if(fixture.values.vacuum >= 0.2){
	PSW[506] = -1;
	*(float*)(PSW+500) = 2.0;
} else{
	abs_x = my_fabs(fixture.values.vacuum);
	exponent = word_log10(abs_x);
	mantissa = abs_x/pow10(exponent);
	if (fixture.values.vacuum < 0){
		mantissa = -mantissa;
	}
	PSW[506] = exponent;
	*(float*)(PSW+500) = mantissa;
}

*(float*)(PSW+600)=DWord_2_Float(fixture.values.azot);
for(i=0; i<6; i++){
	*(float*)(PSW+700+i*10)=fixture.values.temperature[i];
}

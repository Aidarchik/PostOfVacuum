WORD i=0;
valvesState=0;
Write(PLC, 1, MODBUS_RTU_REG_4X, 512, 0, TYPE_WORD, valvesState);
for(i=0; i<countFixtures; i++){
    Write(PLC, 16, MODBUS_RTU_REG_4X, 96+i, 0, TYPE_WORD, 0x01);
}
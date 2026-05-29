#define OWNADDR2 ( (i2c_OAR2 >> 1) & 0x7F )


#define SR1_RESET ( 0x0 )
#define SR1_SB ( (0x1) << 0 )
#define SR1_ADDR ( (0x1) << 1 )
#define SR1_BTF ( (0x1) << 2 )
#define SR1_ADD10 ( (0x1) << 3 )
#define SR1_STOPF ( (0x1) << 4 )
#define SR1_RxNE ( (0x1) << 6 )
#define SR1_TxE ( (0x1) << 7 )
#define SR1_BERR ( (0x1) << 8 )
#define SR1_ARLO ( (0x1) << 9 )
#define SR1_AF ( (0x1) << 10 )
#define SR1_OVR ( (0x1) << 11 )
#define SR1_PECERR ( (0x1) << 12 )
#define SR1_TIMEOUT ( (0x1) << 14 )
#define SR1_SMBALERT ( (0x1) << 15 )


#define SR2_RESET ( 0x0 )
#define SR2_MSL ( (0x1) << 0 )
#define SR2_BUSY ( (0x1) << 1 )
#define SR2_TRA ( (0x1) << 2 )
#define SR2_GENCALL ( (0x1) << 4 )
#define SR2_SMBDEFAULT ( (0x1) << 5 )
#define SR2_SMBHOST ( (0x1) << 6 )
#define SR2_DUALF ( (0x1) << 7 )


#define CR1_RESET ( 0x0 )
#define CR1_PE ( (0x1) << 0 )
#define CR1_SMBUS ( (0x1) << 1 )
#define CR1_SMBTYPE ( (0x1) << 3 )
#define CR1_ENARP ( (0x1) << 4 )
#define CR1_ENPEC ( (0x1) << 5 )
#define CR1_ENGC ( (0x1) << 6 )
#define CR1_NOSTRETCH ( (0x1) << 7 )
#define CR1_START ( (0x1) << 8 )
#define CR1_STOP ( (0x1) << 9 )
#define CR1_ACK ( (0x1) << 10 )
#define CR1_POS ( (0x1) << 11 )
#define CR1_PEC ( (0x1) << 12 )
#define CR1_ALERT ( (0x1) << 13 )
#define CR1_SWRST ( (0x1) << 15 )


#define CR2_RESET ( 0x0 )
#define CR2_ITERREN ( (0x1) << 8 )
#define CR2_ITEVTEN ( (0x1) << 9 )
#define CR2_ITBUFEN ( (0x1) <<10 )
#define CR2_DMAEN ( (0x1) << 11 )
#define CR2_LAST ( (0x1) << 12 )

#define OAR1_RESET ( 0x0 ) 


#define OAR2_RESET ( 0x0 )
#define OAR2_ENDUAL ( (0x1) << 0 )


#define DR_RESET ( 0x0 ) 


#define CCR_RESET ( 0x0 ) 
#define CCR_FS ( (0x1) << 15) 
#define CCR_DUTY ( (0x1) << 14) 


#define TRISE_RESET ( (0x1) << 1 );
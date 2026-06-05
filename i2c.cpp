#include "i2c.h"

void i2c::errorInteruptCB()
{
	it_error_o.write(true);
}

void i2c::eventInteruptCB()
{
	
	it_event_o.write(true);
}

unsigned int i2c::calculateTPCLK(unsigned int freq)
{
	if( ( freq >= 2 ) && ( freq <= 50 ) )
	{
		return ( 1000 / freq);
	}
	else
	return 0;
}

void i2c::pecByteSender(unsigned char pecValue)
{
	m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
	m_sendingTlm.pecValue = pecValue;
	m_AckRelatedOrNot = NOT_ACK_RELATED;
	m_sdaOutPortDriveEvent.notify();
	cout<<"PEC SENDING TLM "<<m_sendingTlm<<endl;
	i2c_CR1 |= CR1_PEC;
	m_stopBitSenderEvent.notify( 9 * clockPeriod_i.read() );

}

void i2c::pecValueUpdate(unsigned char byte)
{
	cout<<this->name() <<"The prev value of pec is "<<hex<<(unsigned int)m_pecValue<<hex<<(unsigned int)byte<<endl; 
	m_pecValue = crcValueUpdate(m_pecValue,byte);
	cout << this->name() <<"THe pecValue is "<<hex<<(unsigned int)m_pecValue<<endl;
}

unsigned char i2c::crcValueUpdate(unsigned char crc , unsigned char byte)
{
	for (int bit = 7; bit >= 0; bit--) 
	{
        unsigned char mix = (crc >> 7) ^ ((byte >> bit) & 1);
        crc <<= 1;
        if (mix) 
			crc ^= 0x07;
    }
    return crc;

}

void i2c::pecValueReset()
{
	m_pecValue = 0x0;
}

/* void i2c::handleSMBAlert()
{
	if(smb_alert_i.read() == false)
	{
		cout<<"handleSMBAlert recevived and is LOW"<<endl;
		if( i2c_CR1 & CR1_SMBTYPE )  // SMBus host mode
        {
            i2c_SR1 |= SR1_SMBALERT;
			if( (i2c_SR1 & SR1_SMBALERT) && (i2c_CR2 & CR2_ITEVTEN) )
			{
				cout<<"The SMBALERT and ITEVTEN so sending interupt"<<endl;
				m_errorEnableIntEvent.notify();;
			}
        }
	}
	else
	{
		cout<<"handleSMBAlert HIGH"<<endl;
	}
}
 */
void i2c::stopBitSender()
{
	cout<<this->name()<<endl;
	cout<<"!!!!!!!!!!!!!!!!!!!!!!CR1 2"<<endl;
	i2c_CR1 &= ~CR1_PEC;
	pecValueReset();
	m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
	m_sendingTlm.stop = true;
	m_AckRelatedOrNot = NOT_ACK_RELATED;
	m_stopCondition = true;
	i2c_SR1 &= ~SR1_TxE;
	i2c_SR1 &= ~SR1_AF;
	m_sdaOutPortDriveEvent.notify();
	m_DRwritten = false;
	i2c_SR2 &= ~SR2_TRA;
	i2c_SR2 &= ~SR2_MSL;
	i2c_SR2 &= ~SR2_SMBDEFAULT;
	i2c_SR2 &= ~SR2_SMBHOST;
	i2c_SR2 &= ~SR2_BUSY;
	//SWReset();
	m_SWResetEvent.notify();	
}

void i2c::SWReset()
{
	i2c_CR1 = CR1_RESET;
	i2c_CR2 = CR2_RESET;
	i2c_OAR1 = OAR1_RESET;
	i2c_OAR2 = OAR2_RESET;
	//i2c_DR = DR_RESET;
	i2c_SR1 = SR1_RESET;
	i2c_SR2 = SR2_RESET;
	i2c_CCR = CCR_RESET;
	i2c_TRISE = TRISE_RESET;
	m_addressingMode = ADDR7;
	m_slaveTransmitOrReceiver = TRANSMIT;
	m_secondAddressAck = false;
	m_sdaAckAssert = false;
	m_AckRelatedOrNot = NOT_ACK_RELATED;
	m_slaveHeaderOrResponsePhase = HEADER;
	m_masterOrSlaveMode = SLAVE_MODE;
	m_startCondition = false;
	m_masterHeaderOrResponsePhase = HEADER; 
	m_masterTransmitOrReceiver = TRANSMIT; 
	m_ackCount = 0;
	m_firstOrSecondHalf = WAIT_FIRST_HALF;
	m_firstHalf10BitAddr =0;
	m_repeatedStartCheck = REPEATED_START_CANNOT_COME;
	m_repeatedStartOrData = DATA;
	m_dataRegYetToRead = false;
	m_DRwritten = false;
	m_SR1ReadDone = false;
	m_ongoingTransmit = false;
	m_ENDUAL = false;
	m_stopCondition = false;
	m_communicationStarted = false;
	m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, 0x0, false, false};
    //sda_o.write(m_sendingTlm);
   // scl_o.write(true);
	m_slaveAddressAckEvent.cancel();
	m_slaveTenBitAddressAckEvent.cancel();
	m_slaveResponsePhaseStartEvent.cancel();
	m_sdaAckDeassertEvent.cancel();
	m_transmitDataEvent.cancel();
	m_updateDataRegEvent.cancel(); 
	m_sdaOutPortDriveEvent.cancel();
	m_masterResponsePhaseStartEvent.cancel();
	m_stopBitSenderEvent.cancel();
	m_SWResetEvent.cancel();
	pecValueReset();
	cout << this->name() << "99999999999999999999999e"<<endl;
}

void i2c::slaveAddressAckEventCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;

	m_AckRelatedOrNot = ACK_RELATED;
	m_sdaAckDeassertEvent.notify();

	// delay for 1 clock period to account for ack bit
	if( m_addressingMode == ADDR10 )
	{
		cout<<"firstorsechalf "<<m_firstOrSecondHalf<<"  Rep startcond sucess in slave"<<m_repeatedStartCheck<<endl;
		if(m_firstOrSecondHalf == WAIT_SECOND_HALF)
		{
			cout<<"GOT First Bit 2 of 10 bit ADDR  !!!!!!!!!!!!! waiting for next"<<endl;
		}
		else if(m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCheck == REPEATED_START_CAN_COME)
		{
			cout<<"Received second part of 10 bit ADDR "<<"!!!!!!!!!!!!!!"<<endl;
		}
		else if(m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCheck == REPEATED_START_FAILED)
		{
			m_slaveResponsePhaseStartEvent.notify();
		}
		else if( m_addressingMode == ADDR10 && m_repeatedStartCheck == REPEATED_START_SUCESS)
		{
			
			m_repeatedStartCondition = false;
			m_slaveResponsePhaseStartEvent.notify();
		}
	}
	else
	{	
		m_slaveResponsePhaseStartEvent.notify();
	}
}

void i2c::ackOrNackCheck(enum ack ackOrNack)
{
	if(ackOrNack == ACK)
	{
		i2c_SR1 &= ~SR1_AF; 
	}
	else
	{
		i2c_SR1 |= SR1_AF;
		i2c_SR1 &= ~SR1_TxE;
		if( (i2c_SR1 & SR1_AF) && (i2c_CR2 & CR2_ITEVTEN) )
		{
			cout<<"The AF and ITEVTEN so sending interupt"<<endl;
			m_errorEnableIntEvent.notify();;
		}
	}
}

void i2c::updateDataRegEventCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	i2cDataTlm receivedDataTlm = sda_i.read();
	if(m_pecEnabled)
	{
		pecValueUpdate( (unsigned char)(receivedDataTlm.data & 0xFF) );
	}
	if(m_dataRegYetToRead == true)
	{
		if( i2c_CR1 & CR1_NOSTRETCH )
		{
			i2c_SR1 = i2c_SR1 | SR1_OVR; // new data arrived before data is read from the dr register so setting OVER RUN. 
			if( (i2c_SR1 & SR1_OVR) && (i2c_CR2 & CR2_ITEVTEN) )
			{
				cout<<"The OVR and ITEVTEN so sending interupt"<<endl;
				m_errorEnableIntEvent.notify();;
			}
			cout << "Same Data DR NOt UPDATED DUE TO OVR " << hex << i2c_DR << endl;
			m_AckRelatedOrNot = ACK_RELATED;
			m_sdaOutPortDriveEvent.notify(clockPeriod_i.read());
			return;
	}
		else
		{
			if( i2c_SR1 & SR1_RxNE )
			{

				i2c_SR1 = i2c_SR1 | SR1_BTF; // new data arrived before data is read from the dr register so setting BTF field 
			}
			if( (i2c_SR1 & SR1_BTF) && (i2c_CR2 & CR2_ITEVTEN) )
			{
				cout<<"The BTF and ITEVTEN so sending interupt"<<endl;
				m_eventEnableIntEvent.notify();;
			}
		}
	}
	else
	{
		i2c_SR1 &= ~SR1_RxNE; // clear RxNe when data is read and new one arrives.
	}

	m_dataRegYetToRead = true; //since new data came so setting true 
	
	i2c_SR1 |= SR1_RxNE; // setting RxNE when new data arrives in DR reg.
	if( (i2c_SR1 & SR1_RxNE) && (i2c_CR2 & CR2_ITEVTEN) && ( i2c_CR2 & CR2_ITBUFEN ))
	{
		cout<<"The RxNE,ITBUFEN and ITEVTEN so sending interupt"<<endl;
		m_eventEnableIntEvent.notify();;
	}
	i2c_DR = receivedDataTlm.data;
	cout << "i2c_DR " << hex << i2c_DR << endl;
	m_AckRelatedOrNot = ACK_RELATED;
	m_sdaOutPortDriveEvent.notify(clockPeriod_i.read());
}

void i2c::sdaOutPortDriveCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;

	if(m_AckRelatedOrNot == ACK_RELATED)
	{
		if( i2c_CR1 & CR1_ACK )
		{ 
			cout<<"SDAOUT ADDRMode "<<m_addressingMode<<endl;
			m_ackCount++;
			// prepare and send the ack
			m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
			m_sendingTlm.addr7OrAddr10 = (m_addressingMode == ADDR7) ? ADDR7 : ADDR10;
			if(m_stopCondition == true )
			{
				cout<<"suuuuuuuuuuuuuuu"<<endl;
				m_sendingTlm.stop = true;
			}
			m_sendingTlm.ackOrNack = ACK;
			m_sendingTlm.ackCount = m_ackCount;
			sda_o.write( m_sendingTlm );
		}
		else
		{
			cout<<"ACK Bit Disabled so no ack"<<endl;
		}
	}
	else if(m_AckRelatedOrNot == NACK_RELATED)
	{
		if( (i2c_SR1 & SR1_PECERR) )
		{
			cout<<"SDAOUT ADDRMode NAACK"<<m_addressingMode<<endl;
			m_ackCount++;
			// prepare and send the ack
			m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
			m_sendingTlm.addr7OrAddr10 = (m_addressingMode == ADDR7) ? ADDR7 : ADDR10;
			m_sendingTlm.ackOrNack = NACK;
			m_sendingTlm.ackCount = m_ackCount;
			sda_o.write( m_sendingTlm );
		}
		else if( i2c_CR1 & CR1_ACK )
		{ 
			cout<<"SDAOUT ADDRMode NACCk"<<m_addressingMode<<endl;
			m_ackCount++;
			// prepare and send the ack
			m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
			m_sendingTlm.addr7OrAddr10 = (m_addressingMode == ADDR7) ? ADDR7 : ADDR10;
			if(m_stopCondition == true )
			{
				m_sendingTlm.stop = true;
			}
			m_sendingTlm.ackOrNack = NACK;
			m_sendingTlm.ackCount = m_ackCount;
			sda_o.write( m_sendingTlm );
		}
		else
		{
			cout<<"ACK Bit in CR is Disables so NACK not send"<<endl;
		}
	}
	else
	{
		sda_o.write( m_sendingTlm );
	}
}

void i2c::transmitDataEventCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
	m_sendingTlm.data = i2c_DR;
	m_AckRelatedOrNot = NOT_ACK_RELATED;
	m_sdaOutPortDriveEvent.notify();

	if( (i2c_SR1 & SR1_BTF) == 0)
	{
		if( m_DRwritten == false )
		{
			i2c_SR1 |= SR1_TxE;
			if( i2c_CR1 & CR1_NOSTRETCH )
			{
				i2c_SR1 |= SR1_OVR;	
				if( (i2c_SR1 & SR1_OVR) && (i2c_CR2 & CR2_ITEVTEN) )
				{
					cout<<"The OVR and ITEVTEN so sending interupt"<<endl;
					m_errorEnableIntEvent.notify();
				}
			}
			if( (i2c_SR1 & SR1_TxE) && (i2c_CR2 & CR2_ITEVTEN) && ( i2c_CR2 & CR2_ITBUFEN ))
			{
				cout<<"The TxE,ITBUFEN and ITEVTEN so sending interupt"<<endl;
				m_eventEnableIntEvent.notify();
			}
		}		
		else
		{
			cout<<"333333333333333333DR False after txCB"<<endl;
			m_DRwritten = false;
		}
	}
	m_ongoingTransmit = true;
}

void i2c::slaveResponsePhase()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	m_slaveHeaderOrResponsePhase = RESPONSE;
	m_stopCondition = false;
	if(m_slaveTransmitOrReceiver == TRANSMIT)
	{
		m_slaveHeaderOrResponsePhase = RESPONSE;
		cout << "!!!!!!!!!!!!!!STARTING SLAVE TRANSMIT" << endl;
		i2c_SR1 |= SR1_TxE;
		//sda_o.write();
		if( (i2c_SR1 & SR1_TxE) && (i2c_CR2 & CR2_ITEVTEN) && ( i2c_CR2 & CR2_ITBUFEN ))
		{
			cout<<"The TxE,ITBUFEN and ITEVTEN so sending interupt"<<endl;
			m_eventEnableIntEvent.notify();;
		}
		m_transmitDataEvent.notify(SC_ZERO_TIME);
	}
	else
	{
		cout << "!!!!!!!!!!!!!!STARTING SLAVE RECEIVE" << endl;
		//sda_i.read();
	}
}

void i2c::masterResponsePhase()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	m_masterHeaderOrResponsePhase = RESPONSE;
	m_startCondition = false;

	if(m_masterTransmitOrReceiver == TRANSMIT)
	{
		cout << "!!!!!!!!!!!!!!STARTING MASTER TRANSMIT" << endl;
		i2c_SR1 |= SR1_TxE;
		//sda_o.write();
		if( (i2c_SR1 & SR1_TxE) && (i2c_CR2 & CR2_ITEVTEN) && ( i2c_CR2 & CR2_ITBUFEN ))
		{
			cout<<"The TxE,ITBUFEN and ITEVTEN so sending interupt"<<endl;
			m_eventEnableIntEvent.notify();;
		}
	}
	else
	{
		cout << "!!!!!!!!!!!!!STARTING MASTER RECEIVE" << endl;
		//sda_i.read();
	}
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << m_masterHeaderOrResponsePhase<<endl;

}

void i2c::sdaInputChangeCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	i2cDataTlm receivedDataTlm = sda_i.read();
	cout<<"MASTER OR SLAVE MODE "<<m_masterOrSlaveMode<<"RESPONSE "<<m_slaveHeaderOrResponsePhase<<endl;

	if(receivedDataTlm.repeatedStart == false && m_startCondition == false && m_repeatedStartCheck == REPEATED_START_CAN_COME)
	{
		m_repeatedStartCondition = false;
		m_repeatedStartCheck = REPEATED_START_FAILED;							
		cout<<"REPEATED START FAIL  TO RECEIVE SO GOING TO RESPONSE PHASE"<<endl;
		slaveResponsePhase();
	}
	else
	{

	}
	if(m_masterOrSlaveMode == SLAVE_MODE)
	{	
		if(receivedDataTlm.pecValue != 0x0)
		{
			if( m_pecValue == receivedDataTlm.pecValue)
			{
				m_AckRelatedOrNot = ACK_RELATED;
				m_sdaAckDeassertEvent.notify( 8 * clockPeriod_i.read() );
				cout<<"There is no error in the data transmited and recived the crc vale is same for both"<<endl;
				cout<<"PEC value calc :"<<hex << (unsigned int)m_pecValue<<" Pec value rec : "<<(unsigned int)receivedDataTlm.pecValue<<endl;
				i2c_SR1 &= ~SR1_PECERR;
			}
			else
			{
				i2c_SR1 |= SR1_PECERR; 
				if( (i2c_SR1 & SR1_PECERR) && (i2c_CR2 & CR2_ITEVTEN) )
				{
					cout<<"The PECERR and ITEVTEN so sending interupt"<<endl;
					m_errorEnableIntEvent.notify();;
				}
				m_AckRelatedOrNot = NACK_RELATED;
				m_sdaAckDeassertEvent.notify( 8 * clockPeriod_i.read() );
				cout<<"There is Error "<<endl;
				cout<<"PEC value calc :"<<hex << (unsigned int)m_pecValue<<" Pec value rec : "<<(unsigned int)receivedDataTlm.pecValue<<endl;
			}
		}
		else if(receivedDataTlm.stop == true)
		{
			cout<<"STOP received in SLAVE"<<endl;
			m_communicationStarted =false;
			m_AckRelatedOrNot = ACK_RELATED;
			m_stopCondition = true;
			i2c_SR1 |= SR1_STOPF;
			i2c_SR2 &= ~SR2_GENCALL;
			m_slaveHeaderOrResponsePhase = HEADER;
			i2c_SR2 &= ~SR2_DUALF;
			i2c_SR2 &= ~SR2_BUSY;
			if( (i2c_SR1 & SR1_STOPF) )
			{
				i2c_SR2 &= ~SR2_TRA;
			}
			if( (i2c_SR1 & SR1_STOPF) && (i2c_CR2 & CR2_ITEVTEN) )
				{
					cout<<"The STOP and ITEVTEN so sending interupt"<<endl;
					m_eventEnableIntEvent.notify();;
				}
		}
		else if( m_slaveHeaderOrResponsePhase == HEADER )
		{
			cout << "RECEIVED data TLM :" << receivedDataTlm << endl;
			
			if( receivedDataTlm.start == true )
			{
				cout<<"!!!!!!!!!!!$$$$$$$$$@@@@@@@@@"<<endl;
				if( receivedDataTlm.address == 0x0c )//alert 
				{
					if(i2c_CR1 & CR1_ALERT)
					{
						cout<<"Alert ADDR recived but CR1ALERT is HIGH "<<endl;
						m_AckRelatedOrNot = NACK_RELATED;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
					}
					else
					{
						i2c_SR1 |= SR1_ADDR;
						i2c_SR1 |= SR1_SMBALERT;
						if( (i2c_SR1 & SR1_SMBALERT) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The SMBALERT and ITEVTEN so sending interupt"<<endl;
							m_errorEnableIntEvent.notify();;
						}
						cout<<"Alert ADDR recived but CR1ALERT is LOW "<<endl;
						m_AckRelatedOrNot = ACK_RELATED;
						m_slaveTransmitOrReceiver = TRANSMIT;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
						i2c_SR1 &= ~SR1_SMBALERT;
						//smb_alert_o.write(true);
						if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();;
						}
					}
				}
				else if( receivedDataTlm.address == 0x61 )//default addr
				{
					if( i2c_CR1 & CR1_ENARP )
					{
						cout << "SMBus Default Address received" << endl;
						i2c_SR2 |= SR2_SMBDEFAULT;
						m_AckRelatedOrNot = ACK_RELATED;
						i2c_SR1 |= SR1_ADDR;
						m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ) ? TRANSMIT : RECEIVE;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
						if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();;
						}
					}
					else
					{
						cout << "SMBus Default Address received But ENARP NOT set" << endl;
						m_AckRelatedOrNot = NACK_RELATED;
						m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ) ? TRANSMIT : RECEIVE;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
					}
				}
				else if( receivedDataTlm.address == 0x08 )//host
				{
					if( ( i2c_CR1 & CR1_SMBTYPE ) && ( i2c_CR1 & CR1_ENARP ) )
					{
						cout << "SMBus Host address received" << endl;
						i2c_SR2 |= SR2_SMBHOST;
						m_AckRelatedOrNot = ACK_RELATED;
						i2c_SR1 |= SR1_ADDR;
						m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ) ? TRANSMIT : RECEIVE;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
						if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();;
						}
					}
					else
					{
						cout << "SMBus Host address received but SMBTYPE/ENARP not set → NACK" << endl;
						m_AckRelatedOrNot = NACK_RELATED;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());		
					}
				}
				else if( (receivedDataTlm.genCall == true ) && (receivedDataTlm.address == 0x0000) )
				{
					if((i2c_CR1 & CR1_ENGC))
					{
						cout<<"got GENERAL CALL 0x00 "<<endl;
						m_AckRelatedOrNot = ACK_RELATED;
						i2c_SR2 |= SR2_GENCALL;
						i2c_SR1 |= SR1_ADDR;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
						//return;
						if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();;
						}
					}
					else
					{
						cout<<"Got General call but ENGC bit failed"<<endl;
						m_AckRelatedOrNot = NACK_RELATED;
						m_sdaAckDeassertEvent.notify(clockPeriod_i.read());
						//return;
					}
				}
				else if( ((receivedDataTlm.addr7OrAddr10 == ADDR7) && (m_addressingMode == ADDR7)) 
						||	((receivedDataTlm.addr7OrAddr10 == ADDR10) && (m_addressingMode == ADDR10)) )
				{
					if(receivedDataTlm.address == 0x0 && receivedDataTlm.genCall == false)
					{
						m_communicationStarted = true;
						i2c_SR2 |= SR2_BUSY;
						cout<<"Master Received a Start Bit "<<endl;
					}
					else if(receivedDataTlm.addr7OrAddr10 == ADDR7)
					{
						bool matchOAR1 = receivedDataTlm.address == getOwnAddress();
						bool matchOAR2 = ( (i2c_OAR2 & OAR2_ENDUAL) && (receivedDataTlm.address == OWNADDR2) );
						if(matchOAR1 || matchOAR2)
						{
							if(matchOAR1)
							{
								cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << " matches to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
								m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ)? TRANSMIT: RECEIVE;
								i2c_SR1 = i2c_SR1 | SR1_ADDR; // setting the bit 1 of SR reg (ADDR) when a matching 7 bit addr comes in. 
								i2c_SR2 &= ~SR2_DUALF;
								m_slaveAddressAckEvent.notify( (1+7+1) * clockPeriod_i.read() );
							}
							else
							{
								cout << "Slave address from OAR2 register 0x" << hex << OWNADDR2 << " matches to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
								m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ)? TRANSMIT: RECEIVE;
								i2c_SR1 = i2c_SR1 | SR1_ADDR; // setting the bit 1 of SR reg (ADDR) when a matching 7 bit addr comes in. 
								i2c_SR2 |= SR2_DUALF;
								cout<<"Setting DAUL FFFFFFFFFFFFFFFFFFFFFFFFFF"<<endl;
								m_slaveAddressAckEvent.notify( (1+7+1) * clockPeriod_i.read() );
							}
							if( m_slaveTransmitOrReceiver == TRANSMIT )
							{
								i2c_SR2 |= SR2_TRA;
							}
							else
							{
								i2c_SR2 &= ~SR2_TRA;
							}
							if(m_pecEnabled)
							{
								pecValueReset();
							}
							if(m_pecEnabled == true)
							{	
								unsigned char addrByte = (receivedDataTlm.address << 1)
                               | (receivedDataTlm.readOrWrite );
								pecValueUpdate( addrByte );
							}
							if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
							{
								cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
								m_eventEnableIntEvent.notify();;
							}
							m_stopCondition = false;
						}
						else
						{
							cout<<"The Reived 7addr does not match with the address of devices "<<hex<<getOwnAddress()<<endl;
							m_AckRelatedOrNot = NACK_RELATED;
							m_sdaAckDeassertEvent.notify();
						}
					}
					else if( receivedDataTlm.addr7OrAddr10 == ADDR10 )
					{
						//IN SLAVE RECIVE THE FIRST 2 BIT of 10 bit addressing
						if(m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCheck == REPEATED_START_CANNOT_COME)
						{
							m_firstHalf10BitAddr = receivedDataTlm.address<<8;
							m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ)? TRANSMIT: RECEIVE;
							m_firstOrSecondHalf = WAIT_SECOND_HALF;
							m_repeatedStartCondition = false;
							m_repeatedStartCheck = REPEATED_START_CANNOT_COME;
							if(m_pecEnabled == true)
								{	
									unsigned char addrByte = (receivedDataTlm.address << 1)
															| ( receivedDataTlm.readOrWrite );
									pecValueUpdate( addrByte );
								}
							m_slaveAddressAckEvent.notify( (7+1+1) * clockPeriod_i.read() );

						}
						//IN SLAVE receveing the rest 8 bit of 10 bit addressing
						else if(m_firstOrSecondHalf == WAIT_SECOND_HALF && m_repeatedStartCheck == REPEATED_START_CANNOT_COME )
						{
							receivedDataTlm.address = receivedDataTlm.address | m_firstHalf10BitAddr;
							m_firstHalf10BitAddr = 0x0;
							if( receivedDataTlm.address == getOwnAddress() && receivedDataTlm.addr7OrAddr10 == ADDR10 )
							{
								m_firstOrSecondHalf = WAIT_FIRST_HALF;	
								m_repeatedStartCheck = REPEATED_START_CAN_COME;
								cout<<"GOT Matching 10 bit addr "<<hex<<getOwnAddress()<<endl;
								i2c_SR1 = i2c_SR1 | SR1_ADDR; // setting the bit 1 of SR reg(ADDR) of slave when a matching 10 bit addr comes in.
								m_slaveAddressAckEvent.notify( (1+7+1) * clockPeriod_i.read() );
								if( m_slaveTransmitOrReceiver == TRANSMIT )
								{
									i2c_SR2 |= SR2_TRA;
								}
								else
								{
									i2c_SR2 &= ~SR2_TRA;
								}
								if(m_pecEnabled == true)
								{	
									unsigned char addrByte = (receivedDataTlm.address << 1)
															| ( receivedDataTlm.readOrWrite );
									pecValueUpdate( addrByte );
								}
								if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
								{
									cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
									m_eventEnableIntEvent.notify();;
								}
							}
							else
							{
								i2c_SR1 = i2c_SR1  & ~SR1_ADDR; // Clear the bit 1 of SR reg (ADDR) when a Wrong 10 bit addr comes in.
								cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << " DOES NOT match to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
							}
						}
					}
					else
					{
						if(receivedDataTlm.address != getOwnAddress())
						{
							i2c_SR1 = i2c_SR1 & ~SR1_ADDR ;
							cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << " DOES NOT match to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
						}
					}
				}
				else
				{
				}
			}
			// else if(receivedDataTlm.repeatedStart == false && m_startCondition == false)
			//  {
			//  	if(m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCheck == REPEATED_START_CAN_COME && receivedDataTlm.repeatedStart == false)
			//  			{
			//  				m_repeatedStartCondition = false;
			//  				m_repeatedStartCheck = REPEATED_START_FAILED;
							
			// 				cout<<"11111111111111HEreeeeeeeeeeeeeeee"<<endl;
			// 				m_slaveResponsePhaseStartEvent.notify();
			//  			}
			//  }
			// Repeat start RECEIVED in slave receive
			else if (receivedDataTlm.repeatedStart == true) 
			{
				m_repeatedStartCondition = true;
				m_repeatedStartCheck = REPEATED_START_SUCESS;
				m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ)? TRANSMIT: RECEIVE;
				if(m_pecEnabled == true)
							{	
								unsigned char addrByte = (receivedDataTlm.readOrWrite );
								pecValueUpdate( addrByte );
							}
				m_slaveAddressAckEvent.notify( (8+1) * clockPeriod_i.read() );
				i2c_SR2 &= ~SR2_DUALF;
				i2c_SR2 &= ~SR2_GENCALL;
				i2c_SR2 &= ~SR2_TRA;
				i2c_SR2 &= ~SR2_SMBDEFAULT;
				i2c_SR2 &= ~SR2_SMBHOST;
			}
			else
			{

			}
		}
		else
		{
			if(m_slaveTransmitOrReceiver == TRANSMIT)
			{
				cout << "Expecting ack in response phase : ";
				cout << receivedDataTlm.ackOrNack << endl; 
				m_ongoingTransmit = false;
				if(receivedDataTlm.ackOrNack == ACK)
				{
					if(receivedDataTlm.stop == true)
					{
						m_slaveHeaderOrResponsePhase = HEADER;
						m_stopCondition = false;
						return;
					}
					if( (i2c_SR1 & SR1_BTF) == 0)
					{
						if( m_DRwritten == true )
						{
							transmitDataEventCB();
							m_ongoingTransmit = true;
						}
						else
						{
							i2c_SR1 |= SR1_TxE;
							i2c_SR1 |= SR1_BTF;
							if( (i2c_SR1 & SR1_BTF) && (i2c_CR2 & CR2_ITEVTEN) )
							{
								cout<<"The BTF and ITEVTEN so sending interupt"<<endl;
								m_eventEnableIntEvent.notify();
							}
							if( (i2c_SR1 & SR1_TxE) && (i2c_CR2 & CR2_ITEVTEN) && (i2c_CR2 & CR2_ITBUFEN) )
							{
								cout<<"The TxE,ITBUFEN and ITEVTEN so sending interupt"<<endl;
								m_eventEnableIntEvent.notify();
							}
						}
					}
					else
					{
						
					}
				}
			}
			else
			{
				cout << "SLAVE Received data is " << hex << receivedDataTlm.data << endl;
				m_updateDataRegEvent.notify( 8 * clockPeriod_i.read() );
			}
		}
		
	}
	else
	{
		if( m_masterHeaderOrResponsePhase == HEADER )
		{
			if(receivedDataTlm.ackOrNack == ACK)
			{
				if(receivedDataTlm.genCall == true)
				{
					cout<<"Got ack in Master after gen call "<<endl;
					i2c_SR1 |= SR1_ADDR;
					return ;
				}
				if(receivedDataTlm.stop == true )
				{
						cout<<"stopoooooooooooooooooooooooo"<<endl;
					m_masterHeaderOrResponsePhase = HEADER;
					m_stopCondition = false;
					return;
				}
				if(receivedDataTlm.addr7OrAddr10 == ADDR10)
				{
					if(m_firstOrSecondHalf == WAIT_SECOND_HALF)
					{
						cout<<"Got ACK in MASTER after getting 2 ADDR bits"<<endl;
					}
					else if(m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCondition == REPEATED_START_CAN_COME)
					{
						i2c_SR1 = i2c_SR1 | SR1_ADDR;
						cout<<sc_time_stamp()<<"Got ACK in MASTER after getting 10 ADDR bits"<<endl;
						if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();;
						}
					}
					else if(m_repeatedStartCheck == REPEATED_START_SUCESS)
					{
						m_repeatedStartCheck = REPEATED_START_CANNOT_COME;
						m_masterResponsePhaseStartEvent.notify();
					}
				}	
				else
				{
					cout<<this->name()<<"Master response notify"<<endl;
					i2c_SR1 = i2c_SR1 | SR1_ADDR;
					m_masterResponsePhaseStartEvent.notify( );
					if( (i2c_SR1 & SR1_ADDR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The ADDR and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();;
						}
				}
			}
			else
			{
				if(receivedDataTlm.ackOrNack == NACK)
				{
					cout<<"NACK msg recived in master "<<endl;
					i2c_SR1 |= SR1_AF;
				}
			}
		}
		else
		{
			if( m_masterTransmitOrReceiver == TRANSMIT )
			{
				if(receivedDataTlm.ackOrNack == ACK)
				{
					if(receivedDataTlm.stop == true)
					{
						cout<<"Master recived stop "<<endl;
						m_masterHeaderOrResponsePhase = HEADER;
						m_stopCondition = false;
						return;
					}
					cout<< sc_time_stamp() << " Master in Response phase, received an ack " << endl;
					m_ongoingTransmit = false;

					if( m_DRwritten == true )
					{
						cout<<"DrWritten true so transmitting"<<endl;
						m_DRwritten = false;
						transmitDataEventCB();
						m_ongoingTransmit = true;
					}
					else
					{
						cout<<"BTF set after data sent"<<endl;
						i2c_SR1 |= SR1_TxE;
						i2c_SR1 |= SR1_BTF;
						if( (i2c_SR1 & SR1_BTF) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The BTF and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();
						}
						if( (i2c_SR1 & SR1_TxE) && (i2c_CR2 & CR2_ITEVTEN) && (i2c_CR2 & CR2_ITBUFEN) )
						{
							cout<<"The TxE,ITBUFEN and ITEVTEN so sending interupt"<<endl;
							m_eventEnableIntEvent.notify();
						}
					}
				}

			}
			else
			{
				if(receivedDataTlm.pecValue != 0x0 )
				{
					if( m_pecValue == receivedDataTlm.pecValue)
					{
						m_AckRelatedOrNot = ACK_RELATED;
						m_sdaAckDeassertEvent.notify( 8 * clockPeriod_i.read() );
						cout<<"There is no error in the data transmited and recived since crc value is same"<<endl;
						cout<<"PEC value calc :"<<hex << (unsigned int)m_pecValue<<" Pec value rec : "<<(unsigned int)receivedDataTlm.pecValue<<endl;
						i2c_SR1 &= ~SR1_PECERR;
					}
					else
					{
						i2c_SR1 |= SR1_PECERR; 
						if( (i2c_SR1 & SR1_PECERR) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"The PECERR and ITEVTEN so sending interupt"<<endl;
							m_errorEnableIntEvent.notify();;
						}
						m_AckRelatedOrNot = NACK_RELATED;
						m_sdaAckDeassertEvent.notify( 8 * clockPeriod_i.read() );
						cout<<"There is  error in the data since crc missmatched "<<endl;
						cout<<"PEC value calc :"<<hex << (unsigned int)m_pecValue<<" Pec value rec : "<<(unsigned int)receivedDataTlm.pecValue<<endl;
					}
				}
				else
				{
					cout << "MASTER Received data is " << hex << receivedDataTlm.data << endl;
					m_updateDataRegEvent.notify( 8 * clockPeriod_i.read() );
				}
			}
		}
	}
}

inline unsigned int i2c::getOwnAddress()
{
	if(m_addressingMode == ADDR7)
	{
		return ( (i2c_OAR1 >> 1) & 0x7F );
	}
	else
	{
		return (i2c_OAR1 & 0x3FF);
	}
}

simple_bus_status i2c::read(int *data
		, unsigned int address)
{
	//  *data = MEM[(address - m_start_address)/4];
	int offset = address - m_start_address;
	switch( offset )
	{
		case 0x0: *data = ( i2c_CR1 & CR1_MASK );
				  break;

		case 0x4: *data = ( i2c_CR2 & CR2_MASK );
				  break;

		case 0x8: *data = ( i2c_OAR1 & OAR1_MASK );
				  break;

		case 0xc: *data = ( i2c_OAR2 & OAR2_MASK );
				  break;

		case 0x10: *data =  ( i2c_DR & DR_MASK );
					if((i2c_CR1 & CR1_PE)==0)
					{
						break;
					}
					cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<"came to read"<<endl;
					m_dataRegYetToRead = false; //clearing since data is read from it;	

					if( ((m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) && (m_masterTransmitOrReceiver == ( RECEIVE ) ) ) ||
					((m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == RESPONSE) && ( m_slaveTransmitOrReceiver == ( RECEIVE ) ) ) )
				   {
					   if( m_SR1ReadDone )
					   {
								cout<<"Clearing BTF________________________"<<endl;
						   i2c_SR1 &= ~SR1_BTF; // BTF field is cleared because of DR read after SR1 read
						   m_SR1ReadDone = false;
					   }
					   if( (i2c_SR1 & SR1_BTF) == 0)
					   {
							i2c_SR1 &= ~SR1_RxNE;
					   }
				   }
				   break;

		case 0x14: *data = ( i2c_SR1 & SR1_MASK );
				    if( i2c_SR1 & SR1_ADDR )
					{
						m_SR1ReadDone = true; 
					}
					if( i2c_SR1 & SR1_SB )     
					{
						m_SR1ReadDone = true;   
					}
					cout<< this->name() << " " <<"Sr1 Read Doneeeeeeeeee 0x"<<i2c_SR1 <<" MasterOrSlave "<<m_masterOrSlaveMode<<endl;
				   break;

		case 0x18: *data = ( i2c_SR2 & SR2_MASK );
					if(m_SR1ReadDone == true)
					{
								cout<<"Clearing ADDR________________________"<<endl;
						i2c_SR1 = i2c_SR1 & ~SR1_ADDR;
						m_SR1ReadDone = false;
					}
					cout<<"SR2 Read Done 0x"<<i2c_SR2<<endl;
				   break;

		case 0x1c: *data = ( i2c_CCR & CCR_MASK );
				   break;

		case 0x20: *data = ( i2c_TRISE & TRISE_MASK );
				   break;

		default:
				   printf("Received address 0x%x does not match the starting address of any of the registers\n", address);
				   break;
	}
	return SIMPLE_BUS_OK;
}

simple_bus_status i2c::write(int *data
		, unsigned int address)
{
	//  MEM[(address - m_start_address)/4] = *data;
	unsigned int receivedOAR1, receivedOAR2;
	int offset = address - m_start_address;
	switch( offset )
	{
		case 0x0: i2c_CR1 = ( *data & CR1_MASK );
				  cout << this->name() << " " << sc_time_stamp() << " CR1 register is being written with value 0x" << hex << i2c_CR1 << endl;
				 
				  if((i2c_CR1 & CR1_PE)==0)
				  {
						i2c_SR1 = SR1_RESET;
						i2c_SR2 = SR2_RESET;
						break;
				  }

				  if(m_SR1ReadDone == true)
				  {
								cout<<"Clearing ADDR________________________"<<endl;
					  m_SR1ReadDone = false;
					  i2c_SR1 &= ~SR1_STOPF;
				  }
				  if( i2c_CR1 & CR1_ENPEC ) 
				  {
					cout<<"############################################PEC enabled"<<endl;
					m_pecEnabled = true;
				  }
				  //Start
				  if(i2c_CR1 & CR1_START)
				  {
					cout<<"Start written"<<endl;
						m_masterOrSlaveMode = MASTER_MODE;
						m_slaveHeaderOrResponsePhase = HEADER;
						m_startCondition = true;
						m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
						m_sendingTlm.start = true;
						m_AckRelatedOrNot = NOT_ACK_RELATED;
						m_sdaOutPortDriveEvent.notify();
						m_communicationStarted = true;
						m_stopCondition = false;
						i2c_SR2 |= SR2_BUSY;
						i2c_SR1 = i2c_SR1 | SR1_SB; //SB field of SR1 register is set on start sent on sda_o line
						cout<<"BBEEEEEEEEP "<<"SR1 DATA "<<i2c_SR1<<"Mode "<<m_masterOrSlaveMode<<endl;
						m_SR1ReadDone = false;
							cout<<"!!!!!!!!!!!!!!!!!!!!!!CR1 1"<<endl;
						m_DRwritten = false;
						i2c_SR2 |= SR2_MSL;
						i2c_SR1 &= ~SR1_TxE;
						i2c_SR1 &= ~SR1_BTF;
						i2c_SR1 &= ~SR1_ADDR;
						i2c_SR1 &= ~SR1_RxNE;
						i2c_SR1 &= ~SR1_STOPF;
						cout<<"BBEEEEEEEEPBBEEEEEEEEP "<<"SR2 DATA "<<i2c_SR2<<"Mode "<<m_masterOrSlaveMode<<endl;
						if(m_pecEnabled == true)
						{
							pecValueReset();
						}
						if( (i2c_SR1 & SR1_SB) && (i2c_CR2 & CR2_ITEVTEN) )
						{
							cout<<"start bit enabled and ITEVTEN is HIGH"<<endl;
							m_eventEnableIntEvent.notify();
						}
						i2c_CR1 &= ~CR1_START;
				  }
				  else if(i2c_CR1 & CR1_STOP)//stop
				  {
					cout<<"STOP written"<<endl;

					  m_communicationStarted =false;
					  i2c_SR1 &= ~SR1_BTF;
					  i2c_SR1 &= ~SR1_TxE;
					  i2c_SR1 &= ~SR1_AF;
					  if(m_pecEnabled)
					  {
						cout<<"Hellyaaaaaaaaaaaaaaaaaaaa"<<endl;
					  	pecByteSender(m_pecValue);
					  }
					  else
					  {
						stopBitSender();
					  }
				  }
				  else
				  {
					  m_masterOrSlaveMode = SLAVE_MODE;
					  m_masterHeaderOrResponsePhase = HEADER;
					  m_slaveHeaderOrResponsePhase = HEADER;
				  }

				 /*  //SMB_alert
				  if(i2c_CR1 & CR1_ALERT)
				  {
					smb_alert_o.write( false );
				  }
				  else
				  {
					smb_alert_o.write( true );
				  } */
				  //Software Reset
				  if(i2c_CR1 & CR1_SWRST )
				  {
	cout << this->name() << "99999999999999999999999e"<<endl;

					SWReset();
					/* if( m_communicationStarted == true )
						{
							stopBitSender();
						} */
					i2c_CR1 &= ~CR1_SWRST;
				  }
				  i2c_CR1 &= ~CR1_START;
				  break;

		case 0x4: i2c_CR2 = ( *data & CR2_MASK );
				  break;

		case 0x8: receivedOAR1 = ( *data & OAR1_MASK );
				  if( receivedOAR1 & (1<<15)) 
				  {
					  // it means 10 bit addressing
					  m_addressingMode = ADDR10;
				  }
				  else
				  {
					  // it means 7 bit addressing
					  m_addressingMode = ADDR7;
				  }
				  i2c_OAR1 = *data ;
				  break;

		case 0xc: receivedOAR2 = ( *data & OAR2_MASK );
				  if( (receivedOAR2 & 0x1) == 1 )
				  {
					m_ENDUAL = true;
				  }
				  else
				  {
					m_ENDUAL = false;
				  }
				  i2c_OAR2 = *data;
				  break;

		case 0x10: i2c_DR = ( *data & DR_MASK );
				    cout << this->name() << " " << sc_time_stamp() << " DR register is being written with value 0x" << hex << i2c_DR << endl;				   	
				  	if((i2c_CR1 & CR1_PE)==0)
					{
						break;
					}
					if( ((m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) && (m_masterTransmitOrReceiver == TRANSMIT)) ||
					((m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == RESPONSE) && (m_slaveTransmitOrReceiver == TRANSMIT)) )
						{
							cout<<"Txe Cleared ____________________"<<endl;
							i2c_SR1 &= ~SR1_TxE; //CLearning when new data comes in .

						}

					if(((i2c_DR & 0xf8) != 0xf0) && m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCheck == REPEATED_START_CAN_COME )
						{
							cout<<"ggggggggggggggggggg"<<endl;
							m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
							m_sendingTlm.start = false;
							m_sendingTlm.addr7OrAddr10 = ADDR10;
							m_sendingTlm.readOrWrite = WRITE; 
							m_sendingTlm.address = i2c_DR;
							m_masterTransmitOrReceiver = TRANSMIT;
							m_repeatedStartCondition = false;
							m_startCondition = false;
							m_repeatedStartCheck = REPEATED_START_FAILED;
							m_AckRelatedOrNot = NOT_ACK_RELATED;
							cout<<"send"<<m_sendingTlm<<endl;
							//m_sdaOutPortDriveEvent.notify();
							//m_masterResponsePhaseStartEvent.notify();
							m_masterHeaderOrResponsePhase = RESPONSE;
							
						}
					else
					{

					}
					if( ((m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) && (m_masterTransmitOrReceiver == ( RECEIVE ) )) ||
					((m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == RESPONSE) && (m_slaveTransmitOrReceiver == ( RECEIVE ) )) )
				   {
					if( m_SR1ReadDone )
					   {
						   cout<<"Clearing BTF ________________________"<<endl;
						   i2c_SR1 &= ~SR1_BTF; // BTF field is cleared because of DR read after SR1 read
						   m_SR1ReadDone = false;
					   }
				   }
						cout<<"~~~~~~~~~~~ IN MODE"<<m_masterOrSlaveMode<<m_slaveTransmitOrReceiver<<m_slaveHeaderOrResponsePhase<<m_masterHeaderOrResponsePhase<<endl;
					if( (m_masterOrSlaveMode == MASTER_MODE) && (m_startCondition == true))
					{	
						//getting the first 2 bits of 8 bit addfrssing
						if( ((i2c_DR & 0xf8) == 0xf0) && m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCheck == REPEATED_START_CANNOT_COME )
						{
							m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
							m_sendingTlm.start = true;
							m_sendingTlm.addr7OrAddr10 = ADDR10;
							m_sendingTlm.address = (i2c_DR >> 1) & 0x3;
							m_sendingTlm.readOrWrite = (i2c_DR & 0x1)? READ: WRITE;

							m_masterTransmitOrReceiver = (i2c_DR & 0x1)? RECEIVE: TRANSMIT;
							m_repeatedStartCondition = false;
							m_firstOrSecondHalf = WAIT_SECOND_HALF;
							m_repeatedStartCheck =REPEATED_START_CANNOT_COME;
							m_repeatedStartCondition = false;
							if(m_SR1ReadDone == true )
							{
								cout<<"Clearing SB_____________________"<<endl;
								i2c_SR1 = i2c_SR1 & ~SR1_SB;
								m_SR1ReadDone = false;
							}
							i2c_SR1 = i2c_SR1 | SR1_ADD10;
							cout<<"SR1 status after getting first half "<<hex<<i2c_SR1<<endl;
							cout<<"Send TLM "<<m_sendingTlm<<endl;


							m_AckRelatedOrNot = NOT_ACK_RELATED;
							if(m_pecEnabled == true)
							{
								unsigned char addrByte = ( m_sendingTlm.address << 1 ) | ( m_sendingTlm.readOrWrite );
								pecValueUpdate( addrByte );
							}

							m_sdaOutPortDriveEvent.notify();
							if( (i2c_SR1 & SR1_ADD10) && (i2c_CR2 & CR2_ITEVTEN) )
							{
								cout<<"The ADD10 and ITEVTEN so sending interupt"<<endl;
								m_eventEnableIntEvent.notify();
							}
						}
						//getting the rest 8 bit sin 10 bit addressing
						else if(m_firstOrSecondHalf == WAIT_SECOND_HALF)
						{
							m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
							m_sendingTlm.start = true;
							m_sendingTlm.addr7OrAddr10 = ADDR10;
							m_sendingTlm.address = i2c_DR;

							m_repeatedStartCondition = false;
							m_firstOrSecondHalf = WAIT_FIRST_HALF;
							m_repeatedStartCheck =REPEATED_START_CAN_COME;
							m_repeatedStartCondition = false;
							if(m_SR1ReadDone == true)
							{
								cout<<"Clearing ADD10_____________________"<<endl;
								i2c_SR1 = i2c_SR1 & ~SR1_ADD10;
								m_SR1ReadDone = false;
							}
							cout<<"Send TLM "<<m_sendingTlm<<endl;

							m_AckRelatedOrNot = NOT_ACK_RELATED;
							if(m_pecEnabled == true)
							{
								unsigned char addrByte = ( m_sendingTlm.address << 1 ) | ( m_sendingTlm.readOrWrite );
								pecValueUpdate( addrByte );
							}
							m_sdaOutPortDriveEvent.notify();
							if(m_masterTransmitOrReceiver == TRANSMIT)
							{
								i2c_SR2 |= SR2_TRA;
							}
							else
							{
								i2c_SR2 &= ~SR2_TRA;
							}
						}
						//getting repeated start
						else if(((i2c_DR & 0xf8) == 0xf0) && ((i2c_DR & 0x01) == 0x01) && m_firstOrSecondHalf == WAIT_FIRST_HALF && m_repeatedStartCondition == REPEATED_START_CAN_COME)
						{
							cout<<"GOT REPEATED START"<<endl;
							m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
							m_repeatedStartCheck = REPEATED_START_SUCESS;
							m_sendingTlm.start = false;						
							m_sendingTlm.readOrWrite = (i2c_DR & 0x1)? READ : WRITE;
							m_masterTransmitOrReceiver = (i2c_DR & 0x1)? RECEIVE : TRANSMIT;
							m_sendingTlm.addr7OrAddr10 = ADDR10;
							m_sendingTlm.repeatedStart = true;
							
							m_repeatedStartCondition = true;

							cout<<"Send TLM "<<m_sendingTlm<<endl;
							m_AckRelatedOrNot = NOT_ACK_RELATED;
							if(m_pecEnabled == true)
							{	
								unsigned char addrByte = ( m_sendingTlm.readOrWrite );
								pecValueUpdate( addrByte );
							}
							m_sdaOutPortDriveEvent.notify();
							i2c_SR2 &= ~SR2_TRA;
						}
						else if( (m_masterOrSlaveMode == MASTER_MODE) && (i2c_DR == 0x0000) )
						{
							cout<<"Gen Call )))))))))))))))"<<endl;
								m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
								m_sendingTlm.genCall = true;
								m_sdaOutPortDriveEvent.notify();
						}
						//7 bit addressing
						else
						{
							cout<<"Entered 7 bit"<<endl;
							m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false, false, 0x0};
							m_sendingTlm.start = true;
							m_sendingTlm.addr7OrAddr10 = ADDR7;
							m_sendingTlm.address = i2c_DR >> 1;
							m_sendingTlm.readOrWrite = (i2c_DR & 0x1)? READ: WRITE;

							m_masterTransmitOrReceiver = (i2c_DR & 0x1)? RECEIVE: TRANSMIT;

							m_AckRelatedOrNot = NOT_ACK_RELATED;
							if(m_pecEnabled == true)
							{
								unsigned char addrByte = ( m_sendingTlm.address << 1 ) | ( m_sendingTlm.readOrWrite );
								pecValueUpdate( addrByte );
							}
							m_sdaOutPortDriveEvent.notify();
							if(m_masterTransmitOrReceiver == TRANSMIT)
							{
								i2c_SR2 |= SR2_TRA;
							}
							else
							{
								i2c_SR2 &= ~SR2_TRA;
							}
							if(m_SR1ReadDone == true )
							{
								cout<<"Clearing SB________________________"<<endl;
								i2c_SR1 = i2c_SR1 & ~SR1_SB;
								m_SR1ReadDone = false;
							}
						}
							
					}
					else if( (m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) )
					{
						cout<<"~~~~~~~~~~~!!ONE"<<endl;
						//m_transmitDataEvent.notify(clockPeriod_i.read());
						m_DRwritten = true;
						i2c_SR1 &= ~SR1_TxE;
						i2c_SR1 &= ~SR1_BTF;
						if(m_pecEnabled == true)
							{
								pecValueUpdate(  (unsigned char)(*data & 0xFF) );
							}
						transmitDataEventCB();
						
					}
					else if( (m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == RESPONSE) && (m_slaveTransmitOrReceiver == TRANSMIT))
					{
						cout<<"~~~~~~~~~~~!!TWO"<<endl;
						m_DRwritten = true;
						i2c_SR1 &= ~SR1_TxE;
						if(m_pecEnabled == true)
							{
								pecValueUpdate( (unsigned char)(*data & 0xFF) );
							}
						transmitDataEventCB();
					}
				   break;

		case 0x14:	i2c_SR1 = *data;
					if( !(*data & SR1_AF) )         
					{
						i2c_SR1 &= ~SR1_AF;          
					}
					if( !(*data & SR1_PECERR) )      
					{
						i2c_SR1 &= ~SR1_PECERR;
					}
					if( !(*data & SR1_OVR) )         
					{
						i2c_SR1 &= ~SR1_OVR;
					}
					if( !(*data & SR1_ARLO) )        
					{
						i2c_SR1 &= ~SR1_ARLO;
					}
					if( !(*data & SR1_BERR) )        
					{
						i2c_SR1 &= ~SR1_BERR;
					}
					break;

		case 0x18: /* i2c_SR2 = *data; */
				   break;

		case 0x1c: i2c_CCR = ( *data & CCR_MASK );
					cout << this->name() << " " << sc_time_stamp() << " CCR value 0x" << hex << (i2c_CCR & CCR_VALUE) << endl;				   	
					m_TPCLK = calculateTPCLK( (i2c_CR2 & CR2_FREQ) );
					if(m_TPCLK == 0)
					break;
					if( i2c_CCR & CCR_FS )
					{
						if( i2c_CCR & CCR_DUTY)
						{
							clockControlTimeReg.tHigh_ns = 9 * ( i2c_CCR & CCR_VALUE) * m_TPCLK ;
							clockControlTimeReg.tLow_ns = 16 * ( i2c_CCR & CCR_VALUE) * m_TPCLK ;
						}
						else
						{
							clockControlTimeReg.tHigh_ns = 1 * ( i2c_CCR & CCR_VALUE) * m_TPCLK ;
							clockControlTimeReg.tLow_ns = 2 * ( i2c_CCR & CCR_VALUE) * m_TPCLK ;
						}
					}
					else
					{
						clockControlTimeReg.tHigh_ns = ( i2c_CCR & CCR_VALUE) * m_TPCLK ;
						clockControlTimeReg.tLow_ns = ( i2c_CCR & CCR_VALUE) * m_TPCLK ;
					}
					break;

		case 0x20: i2c_TRISE = ( *data & TRISE_MASK );
				   break;

		default:
				   printf(" Enough\n ");
				   break;
	}

	return SIMPLE_BUS_OK;
}

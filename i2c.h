/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied.  See the License for the specific language governing
permissions and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  i2c.h : i2c Model class definition

  Original Author: Rajashekar G S, SiliconPatterns, 2026

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

  Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

#ifndef __i2c_h
#define __i2c_h

#include <systemc.h>

#include "simple_bus_types.h"
#include "simple_bus_slave_if.h"

#include "i2c_types.h"
#include "i2c_macros.h"

class i2c
: public simple_bus_slave_if
		, public sc_module
{
		public:
				SC_HAS_PROCESS(i2c);
				// constructor
				i2c(sc_module_name name_
								, unsigned int start_address
								, unsigned int end_address)
						: sc_module(name_)
						  , m_start_address(start_address)
						  , m_end_address(end_address)
		{
				sc_assert(m_start_address <= m_end_address);
				sc_assert((m_end_address-m_start_address+1)%4 == 0);
				i2c_CR1 = 0x0;
				i2c_CR2 = 0x0;
				i2c_OAR1 = 0x0;
				i2c_OAR2 = 0x0;
				i2c_DR = 0x0;
				i2c_SR1 = 0x0;
				i2c_SR2 = 0x0;
				i2c_CCR = 0x0;
				i2c_TRISE = 0x2;
				m_addressingMode = ADDR7;
				m_slaveTransmitOrReceiver = TRANSMIT;
				m_secondAddressAck = false;
				m_sdaAckAssert = false;
				m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, 0x0, false, false};
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
				m_pecEnabled = false;
				m_pecValue = 0x0;
				m_TPCLK = 0x0;
				clockControlTimeReg = { 0x0, 0x0 };
			

				SC_METHOD( sdaInputChangeCB );
				dont_initialize();
				sensitive << sda_i;

				SC_METHOD( slaveAddressAckEventCB);
				dont_initialize();
				sensitive << m_slaveAddressAckEvent << m_slaveTenBitAddressAckEvent;

				SC_METHOD( masterResponsePhase );
				dont_initialize();
				sensitive << m_masterResponsePhaseStartEvent;

				SC_METHOD( slaveResponsePhase );
				dont_initialize();
				sensitive << m_slaveResponsePhaseStartEvent;

				SC_METHOD( transmitDataEventCB );
				dont_initialize();
				sensitive << m_transmitDataEvent;

				SC_METHOD( sdaOutPortDriveCB );
				dont_initialize();
				sensitive << m_sdaAckDeassertEvent << m_sdaOutPortDriveEvent ;

				SC_METHOD( updateDataRegEventCB );
				dont_initialize();
				sensitive << m_updateDataRegEvent;

				/* SC_METHOD( handleSMBAlert );
				dont_initialize();
				sensitive << smb_alert_i;
 */
				SC_METHOD( stopBitSender );
				dont_initialize();
				sensitive << m_stopBitSenderEvent;

				SC_METHOD( SWReset );
				dont_initialize();
				sensitive << m_SWResetEvent;

				SC_METHOD( errorInteruptCB );
				dont_initialize();
				sensitive << m_errorEnableIntEvent;

				SC_METHOD( eventInteruptCB );
				dont_initialize();
				sensitive << m_eventEnableIntEvent;
		}

				// destructor
				~i2c();

				// direct Slave Interface
				bool direct_read(int *data, unsigned int address);
				bool direct_write(int *data, unsigned int address);

				// Slave Interface
				simple_bus_status read(int *data, unsigned int address);
				simple_bus_status write(int *data, unsigned int address);

				unsigned int start_address() const;
				unsigned int end_address() const;

				sc_in< sc_time > clockPeriod_i;
				sc_out< bool > it_event_o;
				sc_out< bool > it_error_o;

				sc_in< i2cDataTlm > sda_i;
				sc_in< i2cSclTlm > scl_i;
				sc_out< i2cDataTlm > sda_o;
				sc_out< i2cSclTlm > scl_o;

				/* sc_in< bool > smb_alert_i;
				sc_out< bool > smb_alert_o; */

		private:
				int i2c_CR1;
				int i2c_CR2;
				int i2c_OAR1;
				int i2c_OAR2;
				int i2c_DR;
				int i2c_SR1;
				int i2c_SR2;
				int i2c_CCR;
				int i2c_TRISE;
				unsigned int m_start_address;
				unsigned int m_end_address;
				enum addr m_addressingMode; 
				enum transmitOrReceiver m_slaveTransmitOrReceiver; 
				bool m_secondAddressAck; 
				bool m_sdaAckAssert;
				i2cDataTlm m_sendingTlm;
				enum ackRelated m_AckRelatedOrNot; 
				enum headerOrResponse m_slaveHeaderOrResponsePhase; 
				enum masterOrSlave m_masterOrSlaveMode;
				bool m_startCondition; 
				enum headerOrResponse m_masterHeaderOrResponsePhase; 
				enum transmitOrReceiver m_masterTransmitOrReceiver; 
				unsigned int m_ackCount;
				enum firstOrSecondHalf10BitAddress m_firstOrSecondHalf;
				unsigned int m_firstHalf10BitAddr;
				bool m_repeatedStartCondition;
				enum repeatedStartCheck m_repeatedStartCheck;
				enum repeatedStartOrData m_repeatedStartOrData;
				bool m_SR1ReadDone;
				bool m_dataRegYetToRead;
				bool m_ongoingTransmit;
				bool m_DRwritten;
				bool m_ENDUAL;
				bool m_stopCondition;
				bool m_communicationStarted;
				bool m_pecEnabled;
				unsigned char m_pecValue;
				clockContolTiming clockControlTimeReg;
				unsigned int m_TPCLK;


				sc_event m_slaveAddressAckEvent;
				sc_event m_slaveTenBitAddressAckEvent;
				sc_event m_slaveResponsePhaseStartEvent;
				sc_event m_sdaAckDeassertEvent;
				sc_event m_transmitDataEvent;
				sc_event m_updateDataRegEvent; 
				sc_event m_sdaOutPortDriveEvent;
				sc_event m_masterResponsePhaseStartEvent;
				sc_event m_stopBitSenderEvent;
				sc_event m_SWResetEvent;
				sc_event m_eventEnableIntEvent;
				sc_event m_errorEnableIntEvent;

				unsigned int getOwnAddress();
				void sdaInputChangeCB();

				void slaveAddressAckEventCB();
				void ackOrNackCheck(enum ack ackOrNack);
				void slaveResponsePhase();
				void sdaOutPortDriveCB(); 
				void transmitDataEventCB();
				void updateDataRegEventCB();
				void masterResponsePhase();
				void stopBitSender();
				void SWReset();
				void handleSMBAlert();
				unsigned char crcValueUpdate(unsigned char crc, unsigned char byte);
				void pecValueReset();
				void pecValueUpdate(unsigned char byte);
				void pecByteSender(unsigned char pecValue);
				unsigned int calculateTPCLK(unsigned int freq);
				void errorInteruptCB();
				void eventInteruptCB();
}; // end class i2c

inline bool i2c::direct_read(int *data, unsigned int address)
{
		return (read(data, address) == SIMPLE_BUS_OK);
}

inline bool i2c::direct_write(int *data, unsigned int address)
{
		return (write(data, address) == SIMPLE_BUS_OK);
}

inline  i2c::~i2c()
{
		//  if (MEM) delete [] MEM;
		//  MEM = (int *)0;
}

inline unsigned int i2c::start_address() const
{
		return m_start_address;
}

inline unsigned int i2c::end_address() const
{
		return m_end_address;
}

#endif
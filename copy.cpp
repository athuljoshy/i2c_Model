#include "i2c.h"

void i2c::sdaInputChangeCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	i2cDataTlm receivedDataTlm = sda_i.read();
    // Repeated start fail detection
    if (receivedDataTlm.repeatedStart == false && m_startCondition == false && m_repeatedStartCheck == REPEATED_START_CAN_COME)
    {
        m_repeatedStartCondition = false;
        m_repeatedStartCheck = REPEATED_START_FAILED;
        slaveResponseTransisitionPhase();  
        return;
    }

    if (m_masterOrSlaveMode == SLAVE_MODE)
    {
		if(receivedDataTlm.pecValue != 0x0)
		{
			handlePEC(receivedDataTlm);
			return;
		}
		else if(receivedDataTlm.stop == true)
		{
			handleSlaveStop();	
			return;
		}

        if (m_slaveHeaderOrResponsePhase == HEADER)
		{	
            slaveHeaderPhase(receivedDataTlm);          
        }
		else
        {   
			 slaveResponsePhase(receivedDataTlm);
		}        
    }
    else  // MASTER_MODE
    {
        if (m_masterHeaderOrResponsePhase == HEADER)
		{
            masterHeaderPhase(receivedDataTlm);
		}         
        else
		{
            masterResponsePhase(receivedDataTlm);      
		}
    }
}


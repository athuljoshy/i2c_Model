#include "simple_bus_master_non_blocking.h"

void simple_bus_master_non_blocking::main_action()
{
	int mydata;
	//		int cnt = 1;
	unsigned int addr = m_start_address;

	wait(); // ... for the next rising clock edge

// master transmit - slave receive (10 BIT)
	mydata = (0x1 << 15) | (0x355); //setting the slave address along with setting the address to be 10 bit format by settind the 15th bit of OAR register.
	addr = 0x40005808;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

	mydata = 0x100; // in master side setting the start bit in master mode
	addr = 0x40005400;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

	mydata = (0xf6) | (0x0); // in master side giving the first two bits of the slave address-xx (11110xx0) along with read or write option,and the 2 bits are saved to a variable in slave side until the rest 8 bits comes in.
	addr = 0x40005410;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

	wait(100, SC_NS);

	mydata = (0x55) ;// in master giving the rest 8 bits of the address and is joined along with the saved 2 bits in the slave side(still in header phase)
	addr = 0x40005410;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

	wait(100, SC_NS);

	mydata = 0xCA; //DATA since data comes in the master goes into the response phase(transmit) and slave into response phase (receive) and then the data tx and rx starts
	addr = 0x40005410;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

	wait(100, SC_NS);


	mydata = 0xAA; //data
	addr = 0x40005410;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

	wait(110, SC_NS);

	mydata = (0x1 << 9);//stop condition.
	addr = 0x40005400;
	bus_port->write(m_unique_priority, &mydata, addr, m_lock);
	while ((bus_port->get_status(m_unique_priority) != SIMPLE_BUS_OK) &&
			(bus_port->get_status(m_unique_priority) != SIMPLE_BUS_ERROR))
		wait();
	if (bus_port->get_status(m_unique_priority) == SIMPLE_BUS_ERROR)
		sb_fprintf(stdout, "%s %s : error on write to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), addr);
	else
		sb_fprintf(stdout, "%s %s : write 0x%x to 0x%x\n",
				sc_time_stamp().to_string().c_str(), name(), mydata, addr);

}

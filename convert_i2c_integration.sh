#!/usr/bin/env bash
set -euo pipefail

# Convert the working-tree I2C integration in orig_i2c_riscvppp to the
# adapter/signal-based integration style used by the under-development tree.
#
# This script is self-contained: it does not require a sibling riscvppp tree.
# It rewrites the relevant files in-place and creates a timestamped backup.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_TREE="${TARGET_TREE:-$ROOT_DIR/riscv-vp-plusplus}"
BACKUP_DIR="${BACKUP_DIR:-$ROOT_DIR/i2c_migration_backup_$(date +%Y%m%d_%H%M%S)}"
EXTERNAL_SIMPLE_BUS_I2C_DIR="${EXTERNAL_SIMPLE_BUS_I2C_DIR:-$ROOT_DIR/Programming/systemc-2.3.4/examples/sysc/simple_bus}"

MAIN_CPP="$TARGET_TREE/vp/src/platform/i2ctest/main.cpp"
CMAKE_FILE="$TARGET_TREE/vp/src/platform/i2ctest/CMakeLists.txt"

if [[ ! -d "$TARGET_TREE" ]]; then
  printf 'target tree not found: %s\n' "$TARGET_TREE" >&2
  exit 1
fi

if [[ ! -d "$EXTERNAL_SIMPLE_BUS_I2C_DIR" ]]; then
  printf 'external SystemC I2C directory not found: %s\n' "$EXTERNAL_SIMPLE_BUS_I2C_DIR" >&2
  printf 'set EXTERNAL_SIMPLE_BUS_I2C_DIR to the correct path and rerun.\n' >&2
  exit 1
fi

mkdir -p "$BACKUP_DIR"
for file in "$MAIN_CPP" "$CMAKE_FILE"; do
  if [[ -f "$file" ]]; then
    rel="${file#"$TARGET_TREE"/}"
    mkdir -p "$BACKUP_DIR/$(dirname "$rel")"
    cp -p "$file" "$BACKUP_DIR/$rel"
  fi
done

mkdir -p "$(dirname "$MAIN_CPP")"
mkdir -p "$(dirname "$CMAKE_FILE")"

cat > "$MAIN_CPP" <<'EOF_MAIN'
#include <boost/program_options.hpp>

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>

#include "core/common/clint.h"
#include "core/common/gdb-mc/gdb_runner.h"
#include "core/rv32/elf_loader.h"
#include "core/rv32/iss.h"
#include "core/rv32/mem.h"
#include "platform/common/bus.h"
#include "platform/common/fe310_plic.h"
#include "platform/common/memory.h"
#include "platform/common/options.h"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "util/options.h"
#include "util/propertytree.h"

#include "i2c.h"

using namespace rv32;
namespace po = boost::program_options;

struct I2CTestOptions : Options {
	typedef uint64_t addr_t;

	addr_t mem_start_addr = 0x80000000ull;
	addr_t mem_size = 0x01000000ull;
	addr_t mem_end_addr = mem_start_addr + mem_size - 1;
	addr_t clint_start_addr = 0x02000000ull;
	addr_t clint_end_addr = 0x0200ffffull;
	addr_t plic_start_addr = 0x40000000ull;
	addr_t plic_end_addr = 0x40ffffffull;
	addr_t i2c0_start_addr = 0x41005400ull;
	addr_t i2c0_end_addr = 0x410054ffull;
	addr_t i2c1_start_addr = 0x41005800ull;
	addr_t i2c1_end_addr = 0x410058ffull;
	addr_t console_start_addr = 0x09004000ull;
	addr_t console_end_addr = 0x09004fffull;
	addr_t exiter_start_addr = 0x09010000ull;
	addr_t exiter_end_addr = 0x09010fffull;

	void parse(int argc, char **argv) override {
		Options::parse(argc, argv);
		mem_end_addr = mem_start_addr + mem_size - 1;
	}
};

struct ConsoleUart : sc_core::sc_module {
	tlm_utils::simple_target_socket<ConsoleUart> tsock;

	explicit ConsoleUart(sc_core::sc_module_name name) : sc_module(name), tsock("tsock") {
		tsock.register_b_transport(this, &ConsoleUart::b_transport);
	}

	void b_transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &) {
		if (trans.get_command() == tlm::TLM_WRITE_COMMAND && trans.get_address() == 0x04u) {
			const char c = static_cast<char>(*trans.get_data_ptr());
			std::cout << c << std::flush;
		}
		trans.set_response_status(tlm::TLM_OK_RESPONSE);
	}
};

struct Exiter : sc_core::sc_module {
	tlm_utils::simple_target_socket<Exiter> tsock;

	explicit Exiter(sc_core::sc_module_name name) : sc_module(name), tsock("tsock") {
		tsock.register_b_transport(this, &Exiter::b_transport);
	}

	void b_transport(tlm::tlm_generic_payload &, sc_core::sc_time &) {
		sc_core::sc_stop();
	}
};

struct I2cRegsAdapter : sc_core::sc_module {
	tlm_utils::simple_target_socket<I2cRegsAdapter, 32> socket{"socket"};
	i2c &dev;

	explicit I2cRegsAdapter(sc_core::sc_module_name name, i2c &dev) : sc_module(name), dev(dev) {
		socket.register_b_transport(this, &I2cRegsAdapter::b_transport);
	}

	void b_transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &) {
		trans.set_dmi_allowed(false);
		if (trans.get_data_ptr() == nullptr || trans.get_data_length() < sizeof(int)) {
			trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
			return;
		}

		int data = 0;
		const unsigned int address = dev.start_address() + static_cast<unsigned int>(trans.get_address());
		switch (trans.get_command()) {
		case tlm::TLM_READ_COMMAND:
			if (!dev.direct_read(&data, address)) {
				trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
				return;
			}
			std::memcpy(trans.get_data_ptr(), &data, sizeof(int));
			trans.set_response_status(tlm::TLM_OK_RESPONSE);
			return;
		case tlm::TLM_WRITE_COMMAND:
			std::memcpy(&data, trans.get_data_ptr(), sizeof(int));
			if (!dev.direct_write(&data, address)) {
				trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
				return;
			}
			trans.set_response_status(tlm::TLM_OK_RESPONSE);
			return;
		default:
			trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
			return;
		}
	}
};

struct BoolIrqBridge : sc_core::sc_module {
	sc_core::sc_in<bool> irq_in{"irq_in"};
	interrupt_gateway *plic = nullptr;
	uint32_t irq_id = 0u;

	SC_HAS_PROCESS(BoolIrqBridge);

	explicit BoolIrqBridge(sc_core::sc_module_name name) : sc_module(name) {
		SC_METHOD(on_irq);
		sensitive << irq_in;
		dont_initialize();
	}

	void on_irq() {
		if (plic != nullptr && irq_in.read()) {
			plic->gateway_trigger_interrupt(irq_id);
		}
	}
};

int sc_main(int argc, char **argv) {
	I2CTestOptions opt;
	opt.parse(argc, argv);

	std::srand(std::time(nullptr));

	if (!opt.property_tree_is_loaded) {
		VPPP_PROPERTY_SET("", "clock_cycle_period", sc_core::sc_time, sc_core::sc_time(10, sc_core::SC_NS));
	}

	tlm::tlm_global_quantum::instance().set(sc_core::sc_time(opt.tlm_global_quantum, sc_core::SC_NS));

	RV_ISA_Config isa_config(opt.use_E_base_isa, opt.en_ext_Zfh);
	ISS core(&isa_config, 0);

	SimpleMemory mem("RAM", opt.mem_size);
	CombinedMemoryInterface iss_mem_if("MemIf", core);
	ELFLoader loader(opt.input_program.c_str());
	FE310_PLIC<1, 16, 32, 32> plic("PLIC");
	CLINT<1> clint("CLINT");
	i2c i2c0("I2C0", opt.i2c0_start_addr, opt.i2c0_end_addr);
	i2c i2c1("I2C1", opt.i2c1_start_addr, opt.i2c1_end_addr);
	I2cRegsAdapter i2c0_regs("I2C0_REGS", i2c0);
	I2cRegsAdapter i2c1_regs("I2C1_REGS", i2c1);
	ConsoleUart console("Console");
	Exiter exiter("Exiter");

	sc_core::sc_signal<sc_core::sc_time> clock_period_signal{"clock_period_signal"};
	sc_core::sc_buffer<i2cDataTlm> i2c0_to_i2c1_sda{"i2c0_to_i2c1_sda"};
	sc_core::sc_buffer<i2cDataTlm> i2c1_to_i2c0_sda{"i2c1_to_i2c0_sda"};
	sc_core::sc_buffer<i2cSclTlm> i2c0_to_i2c1_scl{"i2c0_to_i2c1_scl"};
	sc_core::sc_buffer<i2cSclTlm> i2c1_to_i2c0_scl{"i2c1_to_i2c0_scl"};
	sc_core::sc_buffer<bool> i2c0_event_irq{"i2c0_event_irq"};
	sc_core::sc_buffer<bool> i2c0_error_irq{"i2c0_error_irq"};
	sc_core::sc_buffer<bool> i2c1_event_irq{"i2c1_event_irq"};
	sc_core::sc_buffer<bool> i2c1_error_irq{"i2c1_error_irq"};
	BoolIrqBridge i2c0_event_bridge("I2C0_EVENT_IRQ");
	BoolIrqBridge i2c0_error_bridge("I2C0_ERROR_IRQ");
	BoolIrqBridge i2c1_event_bridge("I2C1_EVENT_IRQ");
	BoolIrqBridge i2c1_error_bridge("I2C1_ERROR_IRQ");

	clock_period_signal.write(sc_core::sc_time(10, sc_core::SC_NS));
	i2c0.clockPeriod_i(clock_period_signal);
	i2c1.clockPeriod_i(clock_period_signal);
	i2c0.sda_o(i2c0_to_i2c1_sda);
	i2c0.sda_i(i2c1_to_i2c0_sda);
	i2c0.scl_o(i2c0_to_i2c1_scl);
	i2c0.scl_i(i2c1_to_i2c0_scl);
	i2c1.sda_o(i2c1_to_i2c0_sda);
	i2c1.sda_i(i2c0_to_i2c1_sda);
	i2c1.scl_o(i2c1_to_i2c0_scl);
	i2c1.scl_i(i2c0_to_i2c1_scl);
	i2c0.it_event_o(i2c0_event_irq);
	i2c0.it_error_o(i2c0_error_irq);
	i2c1.it_event_o(i2c1_event_irq);
	i2c1.it_error_o(i2c1_error_irq);
	i2c0_event_bridge.irq_in(i2c0_event_irq);
	i2c0_error_bridge.irq_in(i2c0_error_irq);
	i2c1_event_bridge.irq_in(i2c1_event_irq);
	i2c1_error_bridge.irq_in(i2c1_error_irq);

	/* Bus: 1 initiator (ISS), 7 targets */
	SimpleBus<1, 7> bus("Bus", nullptr, opt.break_on_transaction);

	{
		unsigned i = 0;
		bus.ports[i++] = new PortMapping(opt.mem_start_addr, opt.mem_end_addr, mem);
		bus.ports[i++] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr, clint);
		bus.ports[i++] = new PortMapping(opt.plic_start_addr, opt.plic_end_addr, plic);
		bus.ports[i++] = new PortMapping(opt.i2c0_start_addr, opt.i2c0_end_addr, i2c0_regs);
		bus.ports[i++] = new PortMapping(opt.i2c1_start_addr, opt.i2c1_end_addr, i2c1_regs);
		bus.ports[i++] = new PortMapping(opt.console_start_addr, opt.console_end_addr, console);
		bus.ports[i++] = new PortMapping(opt.exiter_start_addr, opt.exiter_end_addr, exiter);
	}
	bus.mapping_complete();

	iss_mem_if.isock.bind(bus.tsocks[0]);
	{
		unsigned i = 0;
		bus.isocks[i++].bind(mem.tsock);
		bus.isocks[i++].bind(clint.tsock);
		bus.isocks[i++].bind(plic.tsock);
		bus.isocks[i++].bind(i2c0_regs.socket);
		bus.isocks[i++].bind(i2c1_regs.socket);
		bus.isocks[i++].bind(console.tsock);
		bus.isocks[i++].bind(exiter.tsock);
	}

	std::shared_ptr<BusLock> bus_lock = std::make_shared<BusLock>();
	iss_mem_if.bus_lock = bus_lock;

	MemoryDMI dmi = MemoryDMI::create_start_size_mapping(mem.data, opt.mem_start_addr, mem.get_size());
	InstrMemoryProxy instr_mem(dmi, core);
	iss_mem_if.dmi_add(dmi);
	iss_mem_if.dmi_enable(opt.use_data_dmi);

	instr_memory_if *instr_mem_if = opt.use_instr_dmi ? static_cast<instr_memory_if *>(&instr_mem)
	                                                  : static_cast<instr_memory_if *>(&iss_mem_if);
	data_memory_if *data_mem_if = &iss_mem_if;

	uint64_t entry_point = loader.get_entrypoint();
	try {
		loader.load_executable_image(mem, mem.get_size(), opt.mem_start_addr);
	} catch (ELFLoader::load_executable_exception &e) {
		std::cerr << e.what() << "\nRAM: 0x" << std::hex << opt.mem_start_addr << "\n";
		return -1;
	}

	core.init(instr_mem_if, opt.use_dbbcache, data_mem_if, opt.use_lscache, &clint, entry_point, opt.mem_end_addr);

	/* Interrupt wiring */
	plic.target_harts[0] = &core;
	auto &i2c0_event_bridge_ref = i2c0_event_bridge;
	auto &i2c0_error_bridge_ref = i2c0_error_bridge;
	auto &i2c1_event_bridge_ref = i2c1_event_bridge;
	auto &i2c1_error_bridge_ref = i2c1_error_bridge;
	i2c0_event_bridge_ref.plic = &plic;
	i2c0_event_bridge_ref.irq_id = 1u;
	i2c0_error_bridge_ref.plic = &plic;
	i2c0_error_bridge_ref.irq_id = 2u;
	i2c1_event_bridge_ref.plic = &plic;
	i2c1_event_bridge_ref.irq_id = 3u;
	i2c1_error_bridge_ref.plic = &plic;
	i2c1_error_bridge_ref.irq_id = 4u;
	clint.target_harts[0] = &core;

	new DirectCoreRunner(core);
	opt.handle_property_export_and_exit();
	sc_core::sc_start();
	return 0;
}
EOF_MAIN

cat > "$CMAKE_FILE" <<EOF_CMAKE
set(EXTERNAL_SIMPLE_BUS_I2C_DIR "$EXTERNAL_SIMPLE_BUS_I2C_DIR")

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(SOURCES
	main.cpp
	\${HEADERS}
	\${EXTERNAL_SIMPLE_BUS_I2C_DIR}/i2c.cpp
	\${EXTERNAL_SIMPLE_BUS_I2C_DIR}/i2c_types.cpp)
set(LIBS platform-common gdb-mc
	\${Boost_LIBRARIES}
	systemc
	pthread)

add_executable(i2ctest-vp \${SOURCES})
target_include_directories(i2ctest-vp PRIVATE \${EXTERNAL_SIMPLE_BUS_I2C_DIR})
target_compile_definitions(i2ctest-vp PUBLIC
	TARGET_RV32
	NUM_CORES=1)
target_link_libraries(i2ctest-vp rv32 \${LIBS})
EOF_CMAKE

printf 'updated %s\n' "$MAIN_CPP"
printf 'updated %s\n' "$CMAKE_FILE"
printf 'backup saved in %s\n' "$BACKUP_DIR"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "build_config.h"
#include <platform.h>
#include "config/runtime_config.h"
#include "target.h"

#include "common/streambuf.h"
#include "common/utils.h"

#include "config/parameter_group.h"
#include "config/parameter_group_ids.h"
#include "config/config.h"

#include "drivers/serial.h"
#include "drivers/system.h"

#include "flight/mixer.h"

#include "io/serial.h"
#include "io/msp.h"
#include "io/serial_msp.h"
#include "io/serial_4way.h"
#include "io/serial_bridge.h"

void bridgeSerialProcess(void)
{
	while (serialRxBytesWaiting(mspPorts[0].port)) {
	    uint8_t c = serialRead(mspPorts[0].port);

	    serialBeginWrite(mspPorts[1].port);
	    serialWrite(mspPorts[1].port, c);
	    serialEndWrite(mspPorts[1].port);
	}
	while (serialRxBytesWaiting(mspPorts[1].port)) {
	    uint8_t c = serialRead(mspPorts[1].port);

	    serialBeginWrite(mspPorts[0].port);
	    serialWrite(mspPorts[0].port, c);
	    serialEndWrite(mspPorts[0].port);
	}
}
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
#include "io/msp_protocol.h"

#include "flight/mixer.h"

#include "io/serial.h"
#include "io/msp.h"
#include "io/serial_msp.h"
#include "io/serial_4way.h"
#include "io/serial_bridge.h"

static uint8_t bridgeSerialChecksum(uint8_t checksum, uint8_t byte)
{
    return checksum ^ byte;
}

static uint8_t bridgeSerialChecksumBuf(uint8_t checksum, uint8_t *data, int len)
{
    while(len-- > 0)
        checksum = bridgeSerialChecksum(checksum, *data++);
    return checksum;
}

static bool bridgeSerialProcessReceivedByte(mspPort_t *msp, uint8_t c)
{
    switch(msp->c_state) {
        default:                 // be conservative with unexpected state
        case IDLE:
            if (c != '$')        // wait for '$' to start MSP message
                return false;
            msp->c_state = HEADER_M;
            break;
        case HEADER_M:
            msp->c_state = (c == 'M') ? HEADER_ARROW : IDLE;
            break;
        case HEADER_ARROW:
            msp->c_state = (c == '>' || c == '<') ? HEADER_SIZE : IDLE;
            break;
        case HEADER_SIZE:
            if (c > MSP_PORT_INBUF_SIZE) {
                msp->c_state = IDLE;
            } else {
                msp->dataSize = c;
                msp->offset = 0;
                msp->c_state = HEADER_CMD;
            }
            break;
        case HEADER_CMD:
            msp->cmdMSP = c;
            msp->c_state = HEADER_DATA;
            break;
        case HEADER_DATA:
            if(msp->offset < msp->dataSize) {
                msp->inBuf[msp->offset++] = c;
            } else {
                uint8_t checksum = 0;
                checksum = bridgeSerialChecksum(checksum, msp->dataSize);
                checksum = bridgeSerialChecksum(checksum, msp->cmdMSP);
                checksum = bridgeSerialChecksumBuf(checksum, msp->inBuf, msp->dataSize);
                if(c == checksum)
                    msp->c_state = COMMAND_RECEIVED;
                else
                    msp->c_state = IDLE;
            }
            break;
    }
    return true;
}

static int bridgeMspProcess(mspPacket_t *cmd, mspPacket_t *reply)
{
    sbuf_t *dst = &reply->buf;
    sbuf_t *src = &cmd->buf;
    UNUSED(src);

    reply->cmd = cmd->cmd;

    switch (cmd->cmd) {
        case MSP_IPX: {
            uint8_t sub_cmd = sbufReadU8(src);
            switch(sub_cmd)
            {
                case IPX_NULL:
                    sbufWriteU8(dst, IPX_NULL);
                    break;
                case IPX_UBRIDGE:
                    ipxSerialMode = IPXSERIAL_UBRIDGE;
                    sbufWriteU8(dst, IPX_UBRIDGE);
                    break;
                case IPX_BBRIDGE:
                    ipxSerialMode = IPXSERIAL_BBRIDGE;
                    sbufWriteU8(dst, IPX_BBRIDGE);
                    break;
                case IPX_MSP:
                    ipxSerialMode = IPXSERIAL_MSP;
                    sbufWriteU8(dst, IPX_MSP);
                    break;
                default:
                		reply->result = 0;
                    return 0;
            }
        }
        default:
        		reply->result = 0;
            return 0;   // unknown command
    }
    reply->result = 1;
    return 1;
}

void bridgeSerialProcess(void)
{
	mspPort_t *msp;
	if(ipxSerialMode == IPXSERIAL_UBRIDGE) {
			msp = &mspPorts[0];
	} else if(ipxSerialMode == IPXSERIAL_BBRIDGE) {
			msp = &mspPorts[1];
	} else {
		msp = NULL;
	}
	while (serialRxBytesWaiting(msp->port)) {
	    //uint8_t c = serialRead(msp->port);

	    uint8_t c = serialRead(msp->port);
      bool consumed = bridgeSerialProcessReceivedByte(&mspPorts[0], c);

      if (!consumed && !ARMING_FLAG(ARMED)) {
          evaluateOtherData(msp->port, c);
      }

      if (msp->c_state == COMMAND_RECEIVED) {

      		mspPacket_t command = {
			        .buf = {
			            .ptr = msp->inBuf,
			            .end = msp->inBuf + msp->dataSize,
			        },
			        .cmd = msp->cmdMSP,
			        .result = 0,
			    };
			    static uint8_t outBuf[MSP_PORT_OUTBUF_SIZE];
			    mspPacket_t reply = {
			        .buf = {
			            .ptr = outBuf,
			            .end = ARRAYEND(outBuf),
			        },
			        .cmd = -1,
			        .result = 0,
			    };
			    
			    if(bridgeMspProcess(&command, &reply)) {
			        // reply should be sent back
			        sbufSwitchToReader(&reply.buf, outBuf);     // change streambuf direction
			        serialBeginWrite(msp->port);
					    int len = sbufBytesRemaining(&reply.buf);
					    uint8_t hdr[] = {'$', 'M', reply.result < 0 ? '!' : '>', len, reply.cmd};
					    uint8_t csum = 0;                                       // initial checksum value
					    serialWriteBuf(msp->port, hdr, sizeof(hdr));
					    csum = bridgeSerialChecksumBuf(csum, hdr + 3, 2);          // checksum starts from len field
					    if(len > 0) {
					        serialWriteBuf(msp->port, sbufPtr(&reply.buf), len);
					        csum = bridgeSerialChecksumBuf(csum, sbufPtr(&reply.buf), len);
					    }
					    serialWrite(msp->port, csum);
					    serialEndWrite(msp->port);
			    }else {
		          serialBeginWrite(mspPorts[2].port);
					    int len = sbufBytesRemaining(&command.buf);
					    uint8_t hdr[] = {'$', 'M', '>', len, command.cmd};
					    uint8_t csum = 0;                                       // initial checksum value
					    serialWriteBuf(mspPorts[2].port, hdr, sizeof(hdr));
					    csum = bridgeSerialChecksumBuf(csum, hdr + 3, 2);          // checksum starts from len field
					    if(len > 0) {
					        serialWriteBuf(mspPorts[2].port, sbufPtr(&command.buf), len);
					        csum = bridgeSerialChecksumBuf(csum, sbufPtr(&command.buf), len);
					    }
					    serialWrite(mspPorts[2].port, csum);
					    serialEndWrite(mspPorts[2].port);
			  	}
			    msp->c_state = IDLE;
      }
	}
	while (serialRxBytesWaiting(mspPorts[2].port)) {
			uint8_t c = serialRead(mspPorts[2].port);
      bool consumed = bridgeSerialProcessReceivedByte(&mspPorts[2], c);

      if (!consumed && !ARMING_FLAG(ARMED)) {
          evaluateOtherData(mspPorts[2].port, c);
      }

      if (mspPorts[2].c_state == COMMAND_RECEIVED) {

      		mspPacket_t command = {
			        .buf = {
			            .ptr = mspPorts[2].inBuf,
			            .end = mspPorts[2].inBuf + mspPorts[2].dataSize,
			        },
			        .cmd = mspPorts[2].cmdMSP,
			        .result = 0,
			    };
          serialBeginWrite(msp->port);
			    int len = sbufBytesRemaining(&command.buf);
			    uint8_t hdr[] = {'$', 'M', '<', len, command.cmd};
			    uint8_t csum = 0;                                       // initial checksum value
			    serialWriteBuf(msp->port, hdr, sizeof(hdr));
			    csum = bridgeSerialChecksumBuf(csum, hdr + 3, 2);          // checksum starts from len field
			    if(len > 0) {
			        serialWriteBuf(msp->port, sbufPtr(&command.buf), len);
			        csum = bridgeSerialChecksumBuf(csum, sbufPtr(&command.buf), len);
			    }
			    serialWrite(msp->port, csum);
			    serialEndWrite(msp->port);
			    mspPorts[2].c_state = IDLE;
      }
	}
}
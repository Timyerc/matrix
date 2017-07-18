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
#include "io/beeper.h"

#include "drivers/gpio.h"
#include "drivers/system.h"

#define SET_OSD       GPIO_ResetBits(OSD_DTR_GPIO, OSD_DTR_PIN)
#define RESET_OSD     GPIO_SetBits(OSD_DTR_GPIO, OSD_DTR_PIN)

void serialBridgeInit(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;

	  RCC_AHBPeriphClockCmd(OSD_DTR_PERIPHERAL, ENABLE);
	  GPIO_InitStructure.GPIO_Pin = OSD_DTR_PIN;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(OSD_DTR_GPIO, &GPIO_InitStructure);

	  GPIO_ResetBits(OSD_DTR_GPIO, OSD_DTR_PIN);
}

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
                case IPX_ENDOSDUPLOAD:
                		ipxSerialMode = IPXSERIAL_ENDOSDUPLOAD;
                		sbufWriteU8(dst, IPX_ENDOSDUPLOAD);
                		break;
                default:
                		reply->result = 0;
                    return 0;
            }
        }
        break;
        default:
        		reply->result = 0;
            return 0;   // unknown command
    }
    reply->result = 1;
    return 1;
}

static uint8_t writeOsdRam(void)
{
	uint8_t tranRamBuf1[128] = {0x5f,0x3f,0x90,0x3f,0x96,0x72,0x09,0x00,0x8e,0x16,0xcd,0x60,0x65,0xb6,0x90,0xe7,0x00,0x5c,0x4c,0xb7,0x90,0xa1,0x21,0x26,0xf1,0xa6,0x20,0xb7,0x88,0x5f,0x3f,0x90,0xe6,0x00,0xa1,0x20,0x26,0x07,0x3f,0x8a,0xae,0x40,0x00,0x20,0x0c,0x3f,0x8a,0xae,0x00,0x80,0x42,0x58,0x58,0x58,0x1c,0x80,0x00,0x90,0x5f,0xcd,0x60,0x65,0x9e,0xb7,0x8b,0x9f,0xb7,0x8c,0xa6,0x20,0xc7,0x50,0x5b,0x43,0xc7,0x50,0x5c,0x4f,0x92,0xbd,0x00,0x8a,0x5c,0x9f,0xb7,0x8c,0x4f,0x92,0xbd,0x00,0x8a,0x5c,0x9f,0xb7,0x8c,0x4f,0x92,0xbd,0x00,0x8a,0x5c,0x9f,0xb7,0x8c,0x4f,0x92,0xbd,0x00,0x8a,0x72,0x00,0x50,0x5f,0x07,0x72,0x05,0x50,0x5f,0xfb,0x20,0x04,0x72,0x10,0x00,0x96,0x90,0xa3,0x00};
	uint8_t tranRamBuf2[128] = {0x07,0x27,0x0a,0x90,0x5c,0x1d,0x00,0x03,0x1c,0x00,0x80,0x20,0xae,0xb6,0x90,0xb1,0x88,0x27,0x1c,0x5f,0x3c,0x90,0xb6,0x90,0x97,0xcc,0x00,0xc0,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x81,0xcd,0x60,0x65,0x5f,0x3f,0x97,0x72,0x0d,0x00,0x8e,0x18,0x72,0x00,0x00,0x94,0x0b,0xa6,0x01,0xc7,0x50,0x5b,0x43,0xc7,0x50,0x5c,0x20,0x08,0x35,0x81,0x50,0x5b,0x35,0x7e,0x50,0x5c,0x3f,0x94,0xf6,0x92,0xa7,0x00,0x8a,0x72,0x0c,0x00,0x8e,0x13,0x72,0x00,0x50,0x5f,0x07,0x72,0x05,0x50,0x5f,0xfb,0x20,0x04,0x72,0x10,0x00,0x97,0xcd,0x60,0x65,0x9f,0xb1,0x88,0x27,0x03,0x5c,0x20,0xdb,0x72,0x0d,0x00,0x8e,0x10,0x72};
	uint8_t tranRamBuf3[48] = {0x00,0x50,0x5f,0x07,0x72,0x05,0x50,0x5f,0xfb,0x20,0x24,0x72,0x10,0x00,0x97,0x20,0x1e,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x81};

	mspPort_t *msp;
	if(ipxSerialMode == IPXSERIAL_UBRIDGE || ipxSerialMode == IPXSERIAL_UOSDUPLOAD) {
			msp = &mspPorts[0];
	} else if(ipxSerialMode == IPXSERIAL_BBRIDGE || ipxSerialMode == IPXSERIAL_BOSDUPLOAD) {
			msp = &mspPorts[1];
	} else {
		msp = NULL;
	}

	serialWrite(mspPorts[2].port, 0x31);
	serialWrite(mspPorts[2].port, 0xCE);
	while (!serialRxBytesWaiting(mspPorts[2].port)) {};
	uint8_t c = serialRead(mspPorts[2].port);
	serialWrite(mspPorts[2].port, c);
	if(c == 0x79) {
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0xA0);
		serialWrite(mspPorts[2].port, 0xA0);
		while (!serialRxBytesWaiting(mspPorts[2].port)) {};
		c = serialRead(mspPorts[2].port);
		serialWrite(mspPorts[2].port, c);
		if(c == 0x79) {
			serialWrite(mspPorts[2].port, 0x7F);
			serialWriteBuf(mspPorts[2].port, tranRamBuf1, 128);
			serialWrite(mspPorts[2].port, 0xE8);
			while (!serialRxBytesWaiting(mspPorts[2].port)) {};
			c = serialRead(mspPorts[2].port);
			serialWrite(mspPorts[2].port, c);
			if(c == 0x79) {
			} else {
				serialBeginWrite(msp->port);
				serialWrite(msp->port, 0x11);
				serialEndWrite(msp->port);
				ipxSerialMode = IPXSERIAL_MSP;
				SET_OSD;
				return 0;
			}
		} else {
			serialBeginWrite(msp->port);
			serialWrite(msp->port, 0x12);
			serialEndWrite(msp->port);
			ipxSerialMode = IPXSERIAL_MSP;
			SET_OSD;
			return 0;
		}
	} else {
		serialBeginWrite(msp->port);
		serialWrite(msp->port, 0x13);
		serialEndWrite(msp->port);
		ipxSerialMode = IPXSERIAL_MSP;
		SET_OSD;
		return 0;
	}

	serialWrite(mspPorts[2].port, 0x31);
	serialWrite(mspPorts[2].port, 0xCE);
	while (!serialRxBytesWaiting(mspPorts[2].port)) {};
	c = serialRead(mspPorts[2].port);
	serialWrite(mspPorts[2].port, c);
	if(c == 0x79) {
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x01);
		serialWrite(mspPorts[2].port, 0x20);
		serialWrite(mspPorts[2].port, 0x21);
		while (!serialRxBytesWaiting(mspPorts[2].port)) {};
		c = serialRead(mspPorts[2].port);
		serialWrite(mspPorts[2].port, c);
		if(c == 0x79) {
			serialWrite(mspPorts[2].port, 0x7F);
			serialWriteBuf(mspPorts[2].port, tranRamBuf2, 128);
			serialWrite(mspPorts[2].port, 0xBC);
			while (!serialRxBytesWaiting(mspPorts[2].port)) {};
			c = serialRead(mspPorts[2].port);
			serialWrite(mspPorts[2].port, c);
			if(c == 0x79) {
			} else {
				serialBeginWrite(msp->port);
				serialWrite(msp->port, 0x14);
				serialEndWrite(msp->port);
				ipxSerialMode = IPXSERIAL_MSP;
				SET_OSD;
				return 0;
			}
		} else {
			serialBeginWrite(msp->port);
			serialWrite(msp->port, 0x15);
			serialEndWrite(msp->port);
			ipxSerialMode = IPXSERIAL_MSP;
			SET_OSD;
			return 0;
		}
	} else {
		serialBeginWrite(msp->port);
		serialWrite(msp->port, 0x16);
		serialEndWrite(msp->port);
		ipxSerialMode = IPXSERIAL_MSP;
		SET_OSD;
		return 0;
	}

	serialWrite(mspPorts[2].port, 0x31);
	serialWrite(mspPorts[2].port, 0xCE);
	while (!serialRxBytesWaiting(mspPorts[2].port)) {};
	c = serialRead(mspPorts[2].port);
	serialWrite(mspPorts[2].port, c);
	if(c == 0x79) {
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x01);
		serialWrite(mspPorts[2].port, 0xA0);
		serialWrite(mspPorts[2].port, 0xA1);
		while (!serialRxBytesWaiting(mspPorts[2].port)) {};
		c = serialRead(mspPorts[2].port);
		serialWrite(mspPorts[2].port, c);
		if(c == 0x79) {
			serialWrite(mspPorts[2].port, 0x2F);
			serialWriteBuf(mspPorts[2].port, tranRamBuf3, 48);
			serialWrite(mspPorts[2].port, 0xEA);
			while (!serialRxBytesWaiting(mspPorts[2].port)) {};
			c = serialRead(mspPorts[2].port);
			serialWrite(mspPorts[2].port, c);
			if(c == 0x79) {
			} else {
				serialBeginWrite(msp->port);
				serialWrite(msp->port, 0x17);
				serialEndWrite(msp->port);
				ipxSerialMode = IPXSERIAL_MSP;
				SET_OSD;
				return 0;
			}
		} else {
			serialBeginWrite(msp->port);
			serialWrite(msp->port, 0x18);
			serialEndWrite(msp->port);
			ipxSerialMode = IPXSERIAL_MSP;
			SET_OSD;
			return 0;
		}
	} else {
		serialBeginWrite(msp->port);
		serialWrite(msp->port, 0x19);
		serialEndWrite(msp->port);
		ipxSerialMode = IPXSERIAL_MSP;
		SET_OSD;
		return 0;
	}
	return 1;
}

static void writeOsdGoto(void)
{
	mspPort_t *msp;
	if(ipxSerialMode == IPXSERIAL_UBRIDGE || ipxSerialMode == IPXSERIAL_UOSDUPLOAD) {
			msp = &mspPorts[0];
	} else if(ipxSerialMode == IPXSERIAL_BBRIDGE || ipxSerialMode == IPXSERIAL_BOSDUPLOAD) {
			msp = &mspPorts[1];
	} else {
		msp = NULL;
	}

	serialWrite(mspPorts[2].port, 0x21);
	serialWrite(mspPorts[2].port, 0xDE);
	while (!serialRxBytesWaiting(mspPorts[2].port)) {};
	uint8_t c = serialRead(mspPorts[2].port);
	serialWrite(mspPorts[2].port, c);
	if(c == 0x79) {
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x80);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x80);
		while (!serialRxBytesWaiting(mspPorts[2].port)) {};
		c = serialRead(mspPorts[2].port);
		serialWrite(mspPorts[2].port, c);
		if(c == 0x79) {
			
		} else {
			serialBeginWrite(msp->port);
			serialWrite(msp->port, 0x1F);
			serialEndWrite(msp->port);
			ipxSerialMode = IPXSERIAL_MSP;
			SET_OSD;
			return;
		}
	} else {
		serialBeginWrite(msp->port);
		serialWrite(msp->port, 0x1F);
		serialEndWrite(msp->port);
		ipxSerialMode = IPXSERIAL_MSP;
		SET_OSD;
		return;
	}
}
uint32_t flashAddr = 0x00008000;
void writeOsdFlash(mspPort_t *msp)
{
	serialBeginWrite(mspPorts[2].port);
	serialWrite(mspPorts[2].port, 0x31);
	serialWrite(mspPorts[2].port, 0xCE);
	while (!serialRxBytesWaiting(mspPorts[2].port)) {};
	uint8_t c = serialRead(mspPorts[2].port);
	serialWrite(mspPorts[2].port, c);
	uint8_t checksum = ((flashAddr>>8)&0x0FF) ^ (flashAddr&0x0FF);
	if(c == 0x79) {
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, 0x00);
		serialWrite(mspPorts[2].port, (flashAddr>>8)&0x0FF);
		serialWrite(mspPorts[2].port, flashAddr&0x0FF);
		serialWrite(mspPorts[2].port, checksum);
		while (!serialRxBytesWaiting(mspPorts[2].port)) {};
		c = serialRead(mspPorts[2].port);
		serialWrite(mspPorts[2].port, c);
		if(c == 0x79) {
			serialWrite(mspPorts[2].port, msp->dataSize-2);
			checksum = msp->dataSize-2;
			for(uint8_t i =1; i<msp->dataSize; i++) {
				checksum ^= msp->inBuf[i];
			}
			serialWriteBuf(mspPorts[2].port, &msp->inBuf[1], msp->dataSize-1);
			serialWrite(mspPorts[2].port, checksum);
			while (!serialRxBytesWaiting(mspPorts[2].port)) {};
			c = serialRead(mspPorts[2].port);
			serialWrite(mspPorts[2].port, c);
			if(c == 0x79) {
				serialBeginWrite(msp->port);
				serialWrite(msp->port, 0x79);
				serialEndWrite(msp->port);
				flashAddr += 0x80;
			} else {
				serialBeginWrite(msp->port);
				serialWrite(msp->port, 0x21);
				serialEndWrite(msp->port);
				ipxSerialMode = IPXSERIAL_MSP;
				SET_OSD;
				return;
			}
		} else {
			serialBeginWrite(msp->port);
			serialWrite(msp->port, 0x22);
			serialEndWrite(msp->port);
			ipxSerialMode = IPXSERIAL_MSP;
			SET_OSD;
			return;
		}
	} else {
		serialBeginWrite(msp->port);
		serialWrite(msp->port, 0x23);
		serialEndWrite(msp->port);
		ipxSerialMode = IPXSERIAL_MSP;
		SET_OSD;
		return;
	}
	serialEndWrite(mspPorts[2].port);
}	

void bridgeSerialProcess(void)
{
	mspPort_t *msp;
	if(ipxSerialMode == IPXSERIAL_UBRIDGE || ipxSerialMode == IPXSERIAL_UOSDUPLOAD) {
			msp = &mspPorts[0];
	} else if(ipxSerialMode == IPXSERIAL_BBRIDGE || ipxSerialMode == IPXSERIAL_BOSDUPLOAD) {
			msp = &mspPorts[1];
	} else {
		msp = NULL;
	}

	if(ipxSerialMode == IPXSERIAL_UOSDUPLOAD || ipxSerialMode == IPXSERIAL_BOSDUPLOAD) {
		beeper(BEEPER_SILENCE);
		uint8_t retryCnt=0;
		while(retryCnt++<3) {
			RESET_OSD;
			delay(800);
			serialBeginWrite(mspPorts[2].port);
			serialWrite(mspPorts[2].port, 0x7F);
			while (!serialRxBytesWaiting(mspPorts[2].port)) {};
			uint8_t c = serialRead(mspPorts[2].port);
			serialWrite(mspPorts[2].port, c);
			if(c == 0x79) {
				if(writeOsdRam() != 0) {
					serialBeginWrite(msp->port);
					serialWrite(msp->port, 0x79);
					serialEndWrite(msp->port);
					flashAddr = 0x00008000;
					break;
				} else {
					SET_OSD;
					delay(1000);
				}
			}else {
				SET_OSD;
				delay(1000);
			}
			serialEndWrite(mspPorts[2].port);
		}
		if(retryCnt>=3) {
			serialBeginWrite(msp->port);
			serialWrite(msp->port, 0x10);
			serialEndWrite(msp->port);
			ipxSerialMode = IPXSERIAL_MSP;
			SET_OSD;
			return;
		}
		while(1) {
			while (serialRxBytesWaiting(msp->port)) {
			    uint8_t c = serialRead(msp->port);
		      bool consumed = bridgeSerialProcessReceivedByte(msp, c);

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
							    if(ipxSerialMode == IPXSERIAL_ENDOSDUPLOAD) {
							    		ipxSerialMode = IPXSERIAL_MSP;
							    		writeOsdGoto();
							    		SET_OSD;
							    		return;
							    }
					    }else {
				          if(command.cmd == MSP_IPX) {
				          		if(msp->inBuf[0]==IPX_UOSDUPLOAD || msp->inBuf[0]==IPX_BOSDUPLOAD) {
				          				writeOsdFlash(msp);
				          		}
				          }
					  	}
					    msp->c_state = IDLE;
		      }
			}
		}

	} else {
		SET_OSD;
		while (serialRxBytesWaiting(msp->port)) {
		    //uint8_t c = serialRead(msp->port);

		    uint8_t c = serialRead(msp->port);
	      bool consumed = bridgeSerialProcessReceivedByte(msp, c);

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
}

#ifndef	DEFAULTS_H
#define	DEFAULTS_H

#define	P_SS	B,2
#define	P_MOSI	B,3
#define	P_MISO	B,4
#define	P_SCK	B,5

//#define	MCP2515_CS			D,3	// Rev A
#define	MCP2515_CS			B,2 // Rev B, SparkFun CAN-BUS Shield
//#define	MCP2515_CS			B,1 // SeeedStudio CAN-BUS Shield V2 (default)
#define	MCP2515_INT			D,2
#define LED2_HIGH			B,0
#define LED2_LOW			B,0

#endif	// DEFAULTS_H

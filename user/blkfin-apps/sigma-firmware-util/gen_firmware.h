#define SIGMA_MAGIC "ADISIGM"
#define ADAU_PROGRAM    0x0800
#define ADAU_PARAMS     0x0000

#define MAX_LEN (4096)
#define OUTPUT_FILE "adau1761.bin"

typedef unsigned short     u16;
typedef unsigned char	   u8;
typedef unsigned int	   u32;

struct sigma_firmware_header {
	unsigned char magic[7];
	u8 version;
	u32 crc;
};

enum {
	SIGMA_ACTION_WRITEXBYTES = 0,
	SIGMA_ACTION_WRITESINGLE,
	SIGMA_ACTION_WRITESAFELOAD,
	SIGMA_ACTION_DELAY,
	SIGMA_ACTION_PLLWAIT,
	SIGMA_ACTION_NOOP,
	SIGMA_ACTION_END,
};

struct sigma_action {
	u8 instr;
	u8 len_hi;
	u16 len;
	u16 addr;
	unsigned char payload[];
};


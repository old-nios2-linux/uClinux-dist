u16 dm8606_read(board_info_t *, int);
void dm8606_write(board_info_t *, int, u16);

u16 dm8606_read(board_info_t *db, int offset)
{
	int bval, i=0;
	bval = ( offset >> 5 ) | 0x80;
	iow(db, 0x33, bval);
	bval = offset;
	iow(db, 0x0c, bval);
	
	iow(db, 0xb, 0x8); 	/* Clear phyxcer read command */
	iow(db, 0xb, 0xc); 	/* Issue phyxcer read command */
	iow(db, 0xb, 0x8); 	/* Clear phyxcer read command */
	
	do {
		if ((!(ior(db,0xb) & 0x1)) || i>1000)
			break;
		i++;
	}while(1);
	iow(db, 0x33, 0);
	/* The read data keeps on REG_0D & REG_0E */
	return ( ior(db, DM9KS_EPDRH) << 8 ) | ior(db, DM9KS_EPDRL);
	
}

void dm8606_write(board_info_t *db, int offset, u16 value)
{
	int bval,i=0;
	bval = ( offset >> 5 ) | 0x80;
	iow(db, 0x33, bval);
	bval = offset;
	iow(db, 0x0c, bval);
	
	/* Fill the written data into REG_0D & REG_0E */
	iow(db, 0xd, (value & 0xff));
	iow(db, 0xe, ( (value >> 8) & 0xff));

	iow(db, 0xb, 0x8);	/* Clear phyxcer write command */
	iow(db, 0xb, 0xa);	/* Issue phyxcer write command */
	iow(db, 0xb, 0x8);	/* Clear phyxcer write command */
	do {
		if ((!(ior(db,0xb) & 0x1)) || i >1000)
			break;
		i++;
	}while(1);
	iow(db, 0x33, 0);
}

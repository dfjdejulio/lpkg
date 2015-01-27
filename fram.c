#include <stdio.h>

#define SYN 0x16
#define DLE 0x10
#define STX 0x02
#define ETX 0x03

extern FILE* newton;

inline void fcscalc(unsigned short *fcs, unsigned char data) {
	*fcs = updcrc(*fcs, &data, 1);
}

int
read_frame(char *buff) {
	char buf=SYN;
	int i=0;
	char *org=buff;
	unsigned short fcs=0;
	unsigned short fcs_rec=0;

	for(;buf==SYN;buf=getc(newton));
	if(buf==DLE && getc(newton)==STX) {
		for(;;) {
			buf=getc(newton);
			if(buf==DLE) {
				buf=getc(newton);
				if(buf==ETX) {
					fcscalc(&fcs,buf);
					fcs_rec=getc(newton);
					fcs_rec|=getc(newton)<<8;
					if(fcs_rec==fcs) {
						return(i);
					} else {
						fprintf(stderr,"Bad CRC\n");
						fflush(stderr);
						return -1;
					}
				} else if(buf==DLE) {
					*buff++=buf;
					fcscalc(&fcs,buf);
					i++;
				} else {
					fprintf(stderr,"Invalid frame\n");
					fflush(stderr);
					return -1;
				}
			} else {
				*buff++=buf;
				fcscalc(&fcs,buf);
				i++;
			}
		}
	}
}

send_frame(char *buff, int size) {
	int i;
	unsigned short fcs = 0;

	putc(SYN,newton);
	putc(DLE,newton);
	putc(STX,newton);

	for(i=0;i<size;i++) {
		putc(buff[i],newton);
		if(buff[i]==DLE)
			putc(DLE,newton);
		fcscalc(&fcs, buff[i]);
	}

	putc(DLE,newton);
	putc(ETX,newton);
	fcscalc(&fcs, ETX);
	putc(fcs&0x00ff,newton);
	putc((fcs&0xff00)>>8,newton);
	fflush(newton);
}

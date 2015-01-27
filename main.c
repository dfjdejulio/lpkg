/*
    lpkg - tool for downloading packages to Newton
    Copyright (C) 1998 Filip R. Zawadiak <philz@silesia.linux.org.pl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define LR  1
#define LD  2
#define LT  4
#define LA  5
#define LN  6
#define LNA 7

sendLA(unsigned char nr,unsigned char cr) {
	char la[] = {3,5,0,0};
	la[2]=nr;
	la[3]=cr;
	send_frame(la, sizeof(la));
}

unsigned char s;
unsigned char r;
FILE* newton;

int
sendData(char *data, int size) {
	unsigned char buf[2048];
	unsigned char buf2[8];
	int i;

	buf[0]=2;
	buf[1]=4;
	buf[2]=s;

	for(i=0;i<size;i++)
		buf[i+3]=data[i];

	for(;;)	{
		send_frame(buf,size+3);
		read_frame(buf2,sizeof(buf2));
		if(buf2[0]==3 && buf2[1]==5 && buf2[2]==s) {
			s++;
			return size;
		}
	}
}

int
receiveData(char *data, int size) {
	char buf[2048];
	int s,i;

	for(;;) {
		s=read_frame(buf);
		if(s>0) {
			sendLA(r++,1);
			s=s>size?size:s;
			for(i=0;i<s;i++)
				data[i]=buf[i+3];
			return s;
		}
	}
}

int
waitConnection() {
	char lr[]={23,1,2,1,6,1,0,0,0,0,255,2,1,2,3,1,1,4,2,64,0,8,1,3};
	char buf[64];

	while(read_frame(buf)<0);
	send_frame(lr, sizeof(lr));
	r=1;
	read_frame(buf);
	s=1;
}

int
sendDisconnect() {
	char ld[]={7,2,1,1,255,2,1,0};
	send_frame(ld, sizeof(ld));
}

void
usage() {
	fprintf(stderr,"usage: lpkg [-d device] [-f] package.pkg\n");
	exit(-1);
}

int
main(int argc, char* argv[]) {
	int i,j;
	int fd,nw,r;
	int speed=B38400;
	char *dev="/dev/newton";
	char *pkg=0;
	char opt;
	unsigned int size;
	struct stat st;
	struct termios ts;
	char* package;
	char lt[]={'n','e','w','t','d','o','c','k','d','o','c','k',0,0,0,4,0,0,0,4};
	char lt2[]={'n','e','w','t','d','o','c','k','s','t','i','m',0,0,0,4,0,0,0,5};
	char lt3[]={'n','e','w','t','d','o','c','k','l','p','k','g',0,0,0x2f,0xa0};
	char lt4[]={'n','e','w','t','d','o','c','k','d','i','s','c',0,0,0,0};
	char buf[128];

	if(argc<2)
		usage();

	while((opt=getopt(argc,argv,"hd:f"))!=-1) {
		switch(opt) {
			case 'd':
				dev = optarg;
				break;
			case 'f':
				speed = B115200;
				break;
			default:
				usage();
		}
	}

	if(optind==argc) 
		usage();

	pkg = argv[optind];
		
	nw=open(dev,O_RDWR);
	if(nw<0) {
		fprintf(stderr,"Cannot open %s\n",dev);
		exit(-1);
	}
	cfmakeraw(&ts);
	cfsetospeed(&ts,speed);
	cfsetispeed(&ts,speed);
	tcsetattr(nw,TCSANOW,&ts);

	newton = fdopen(nw,"r+");
	
	fd=open(pkg,O_RDONLY);
	if(fd<0) {
		fprintf(stderr,"Cannot open package %s\n",pkg);
		exit(-1);
	}

	fstat(fd,&st);
	*(lt3+12)=htonl(st.st_size);
	/* (unsigned long)*(lt3+12)=htonl(st.st_size); */

	fprintf(stderr,"Waiting...");
	waitConnection();
	fprintf(stderr,"\rConnecting...");
	receiveData(buf,sizeof(buf));	// newtdockrtdk
	sendData(lt,sizeof(lt));	// newtdockdock
	receiveData(buf,sizeof(buf));	// newtdockname
	sendData(lt2,sizeof(lt2));	// newtdockstim
	receiveData(buf,sizeof(buf));	// newtdockdres
	fprintf(stderr,"\rLoading %s, %d bytes\n",argv[1],st.st_size);
	sendData(lt3,sizeof(lt3));	// newtdocklpkg

	r=read(fd,buf,sizeof(buf));
	fprintf(stderr,"\r%d",size=r);
	sendData(buf,r);
	while(r==sizeof(buf)) {
		r=read(fd,buf,sizeof(buf));
		fprintf(stderr,"\r%d",size+=r);
		sendData(buf,r);
	}

	receiveData(buf,sizeof(buf));	// newtdockdres

	sendData(lt4,sizeof(lt4));	// newtdockdisc
	sendDisconnect();
	fprintf(stderr,"\n");
	fflush(stderr);
}

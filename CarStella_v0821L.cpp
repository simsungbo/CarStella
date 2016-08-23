#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/poll.h>


#define TERMINAL_DEBUG
#define PRTCL_INIT					96	// ASCII CODE: '`'
#define PRTCL_START					126	// ASCII CODE: '~'
#define PRTCL_COMM_SET_SETLED		0
#define PRTCL_COMM_SET_SETDCM		1
#define PRTCL_COMM_SET_SETSERVO		2
#define PRTCL_COMM_REQ_TEST			100
#define PRTCL_COMM_REQ_IR			101
#define PRTCL_COMM_RESP_TEST		200
#define PRTCL_COMM_RESP_OK			201
#define PRTCL_COMM_RESP_IR			202
#define PRTCL_LENGTH_MAX			14
#define PRTCL_LENGTH_MIN			1


int OpenSerial( char *dev_name, int baud, int vtime, int vmin );
void CloseSerial( int fd );
void SetLed(int fd, int totNum, unsigned char* ledNum, unsigned char* ledMode);
void SetDCM(int fd, unsigned char dir, unsigned char speed);
void SetServo(int fd, unsigned char deg);
int ReqTest(int fd);
int ReqIR(int fd, int totNum, unsigned char* srcIRNum, unsigned char* dstIRVal);
#ifdef TERMINAL_DEBUG
int TestComm(int fd);
void TestReadAToZ(int fd);
void TestWrite1or0(int fd);
void TestAutoReqTest(int fd);
#endif

int main(int argc, char* argv[])
{
	int fd;
	char dev_name[128] = "/dev/ttyACM0";
	char cc, buf[128];
	int rdcnt;
	
	fd = OpenSerial( dev_name, 115200, 10, 1 );
	#ifdef TERMINAL_DEBUG
	printf( "Serial test start\n" );
	#endif
	// To Do Start ///////////////////////////////////////////////////////////
	
	// communication test 
	while(1)
	{
		if(TestComm(fd) == -1) break;
	}
	
	// To Do End ////////////////////////////////////////////////////////////
	#ifdef TERMINAL_DEBUG
	printf( "Serial test end\n" );
	#endif

	CloseSerial( fd );

	return 0;
}


int OpenSerial( char *dev_name, int baud, int vtime, int vmin )
{
	int fd;
	char ch;
	struct termios newtio;
	
	// open the serial port
	fd = open( dev_name, O_RDWR | O_NOCTTY );
	if ( fd < 0 ) {
		#ifdef TERMINAL_DEBUG
		printf( "Device OPEN FAIL %s\n", dev_name );
		#endif
		return -1;
	}

	// set the termios structure (115200 baudrate, 8N1)
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts
	switch( baud )
	{
		case 115200 : newtio.c_cflag |= B115200; break;
		case 57600  : newtio.c_cflag |= B57600;  break;
		case 38400  : newtio.c_cflag |= B38400;  break;
		case 19200  : newtio.c_cflag |= B19200;  break;
		case 9600   : newtio.c_cflag |= B9600;   break;
		case 4800   : newtio.c_cflag |= B4800;   break;
		case 2400   : newtio.c_cflag |= B2400;   break;
		default     : newtio.c_cflag |= B115200; break;
	}

	// set the termios structure (input mode: non-canonical, no echo,.....)
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = vtime; 
	newtio.c_cc[VMIN] = vmin;
	usleep(5000 * 1000);	// wait arduino initialize
	tcflush ( fd, TCIOFLUSH );
	tcsetattr( fd, TCSANOW, &newtio );

	// send the odroid initialize success
	ch = PRTCL_INIT;
	write( fd, &ch, 1);

	return fd;
}


void CloseSerial( int fd )
{
	close( fd );
}


void SetLed(int fd, int totNum, unsigned char* ledNum, unsigned char* ledMode)
{
	unsigned char comm[(totNum*2)+4];
	/*
	comm[0] = PRTCL_START;
	comm[1] = (totNum*2) + 2;
	comm[2] = PRTCL_COMM_SET_SETLED;
	comm[3] = totNum;
	for(int i=0; i<totNum; i++)
	{
		comm[(i*2)+4] = ledNum[i];
		comm[(i*2)+5] = ledMode[i];
	}

	write( fd, comm, (totNum*2)+4 );
	*/
	return;
}


void SetDCM(int fd, unsigned char dir, unsigned char speed)
{
	unsigned char comm[5];

	comm[0] = PRTCL_START;
	comm[1] = 3;
	comm[2] = PRTCL_COMM_SET_SETDCM;
	comm[3] = dir;
	comm[4] = speed;
	
	write( fd, comm, 5 );

	return;
}


void SetServo(int fd, unsigned char deg)
{
	unsigned char comm[4];

	comm[0] = PRTCL_START;
	comm[1] = 2;
	comm[2] = PRTCL_COMM_SET_SETSERVO;
	comm[3] = deg;
	
	write( fd, comm, 4 );

	return;
}


int ReqTest(int fd)
{
	unsigned char comm[3] = {0};
	int rdcnt = 0;
	unsigned char buf[4] = {0};

	comm[0] = PRTCL_START;
	comm[1] = 1;
	comm[2] = PRTCL_COMM_REQ_TEST;

	write( fd, comm, 3 );

	while(rdcnt < 4) {
		ioctl(fd, FIONREAD, &rdcnt);
	}
	rdcnt = read( fd, buf, 4 );

	if ( rdcnt != 4 ) {
		#ifdef TERMINAL_DEBUG
		printf("rdcnt = %d \n", rdcnt);
		#endif
		return -1;
	}
	if ( buf[0] != PRTCL_START )			return -2;
	if ( buf[1] != 2 )						return -3;
	if ( buf[2] != PRTCL_COMM_RESP_TEST )	return -4;
	if ( buf[3] != PRTCL_COMM_REQ_TEST )	return -5;

	return 0;
}


int ReqIR(int fd, int totNum, unsigned char* srcIRNum, unsigned char* dstIRVal)
{
	unsigned char comm[totNum+4];
	unsigned char buf[(totNum*2)+4];
	int rdcnt = 0;
	/*
	comm[0] = PRTCL_START;
	comm[1] = totNum + 2;
	comm[2] = PRTCL_COMM_REQ_IR;
	comm[3] = totNum;
	for(int i=0; i<totNum; i++)
		comm[i+4] = srcIRNum[i];

	write( fd, comm, totNum+4 );

	while( rdcnt < ((totNum*2)+4) ) {
		ioctl(fd, FIONREAD, &rdcnt);
	}
	
	rdcnt = read( fd, buf, ((totNum*2)+4) );
	if( rdcnt != ((totNum*2)+4) ) return -1;

	for(int i=0; i<totNum; i++)
		dstIRVal[i] = buf[(i*2)+5];
	*/
	return 0;
}

#ifdef TERMINAL_DEBUG
int TestComm(int fd)
{
	char ch;
	int temp;
	unsigned char ucTemp1 ,ucTemp2;
	char str[200];
	printf("1-ReqTest/2-DC/3-Servo/q-quit: ");
	ch = getchar(); getchar();

	if(ch == 'q')	return -1;

	switch(ch)
	{
		case '1': printf("1!\n");
			temp = ReqTest(fd);
			if(temp == 0)	printf("ReqTest success!\n");
			else			printf("ReqTest fail.. %d \n", temp);
			break;

		case '2': printf("2!\n");
			printf("set dir(0-Stop/1-Break/2-CW/3-CCW/4-Standby): ");
			ucTemp1 = getchar() - 48;	getchar();
			printf("set speed(0~255): ");
			scanf("%s", str);	getchar();
			ucTemp2 = atoi(str);
			SetDCM(fd, ucTemp1, ucTemp2);
			break;

		case '3': printf("3!\n");
			printf("set deg(~90~): ");
			scanf("%s", str);	getchar();
			ucTemp1 = atoi(str);
			SetServo(fd, ucTemp1);
			break;

		default:  printf("default!\n");
			break;
	}
	return 0;
}
void TestReadAToZ(int fd)
{
	while(1)
	{
		char buf[128] = {0};
		int rdcnt;

		rdcnt = read(fd, buf, 1);
		if(rdcnt > 0) printf("%c", buf[0]);
		else printf("error\n");
	}
	
	return;
}
void TestWrite1or0(int fd)
{
	char ch;
	
	while(1) {
		printf("input(1-led~/0-ledOFF): ");
		ch = getchar(); getchar();
		write(fd, &ch, 1);
	}

	return;
}
void TestAutoReqTest(int fd)
{
	int temp = 0;
	while(1) {
			temp = ReqTest(fd);
			
			if(temp == 0)
				printf("ReqTest success!\n");
			else {
				printf("ReqTest fail.. %d \n", temp);
				break;
			}
	}
	return;
}
#endif

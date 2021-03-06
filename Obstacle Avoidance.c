#include<stdio.h>	//printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <time.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include "pca9685/pca9685.h"
#include <wiringPi.h>
#include <signal.h>
#define PIN_BASE 300
#define MAX_PWM 4096
#define HERTZ 50
int fd;
#define BUFLEN 512	//Max length of buffer
#define PORT 8888	//The port on which to listen for incoming data
/*
 * wiringPi C library use different GPIO pin number system from BCM pin numberwhich are often used by Python, 
 * you can lookup BCM/wPi/Physical pin relation by following Linux command : gpio readall
 */
 //define L298N control pins in wPi system
#define ENA 0  //left motor speed pin ENA connect to PCA9685 port 0
#define ENB 1  //right motor speed pin ENB connect to PCA9685 port 1
#define IN1 4  //Left motor IN1 connect to wPi pin# 4 (Physical 16,BCM GPIO 23)
#define IN2 5  //Left motor IN2 connect to wPi pin# 5 (Physical 18,BCM GPIO 24)
#define IN3 2  //right motor IN3 connect to wPi pin# 2 (Physical 13,BCM GPIO 27)
#define IN4 3  //right motor IN4 connect to wPi pin# 3 (Physical 15,BCM GPIO 22)

// Define GPIO to use on Pi
#define GPIO_TRIGGER 20
#define GPIO_ECHO 21

// Ultrasonic sensor facings
#define servo_left 420
#define servo_center 300
#define servo_right 180

/*IF THIS IS FOR THE LINE TRACKING THEN THIS CODE IS NOT NECESSARY
//define IR tracking sensor wPi pin#
#define sensor1 21 // No.1 sensor from far left to wPi#21 Physical pin#29
#define sensor2 22 // No.2 sensor from left to wPi#22 Physical pin#31
#define sensor3 23 // middle sensor to wPi#23 Physical pin#33
#define sensor4 24 // No.2 sensor from right to wPi#24 Physical pin#35
#define sensor5 25 // No.1 sensor from far  right to wPi#25 Physical pin#37
char val[5]; //sensor value array
*/

#define high_speed 3000  // Max pulse length out of 4096
#define mid_speed  2000  // Max pulse length out of 4096
#define low_speed  1350  // Max pulse length out of 4096
#define high_speed_2 4000  // Max pulse length out of 4096
#define mid_speed_2  2500  // Max pulse length out of 4096
#define low_speed_2  2000  // Max pulse length out of 4096
#define short_delay 200
#define long_delay 250
#define extra_long_delay 600

void setup(){
 pinMode(IN1,OUTPUT);
 pinMode(IN2,OUTPUT);
 pinMode(IN3,OUTPUT);
 pinMode(IN4,OUTPUT);
 pinMode(GPIO_TRIGGER, OUTPUT);
 pinMode(GPIO_ECHO, INPUT);
/* BELIEVE THIS IS FOR THE LINE TRACKER AND THEREFORE UNNECESSARY
 pinMode(sensor1,INPUT);
 pinMode(sensor2,INPUT);
 pinMode(sensor3,INPUT);
 pinMode(sensor4,INPUT);
 pinMode(sensor5,INPUT);
*/  
 
 digitalWrite(IN1,LOW);
 digitalWrite(IN2,LOW);
 digitalWrite(IN3,LOW);
 digitalWrite(IN4,LOW);
}
void go_Back(int fd,int l_speed,int r_speed){
    digitalWrite(IN1,HIGH);
    digitalWrite(IN2,LOW);
    digitalWrite(IN3,HIGH);
    digitalWrite(IN4,LOW); 
    pca9685PWMWrite(fd, ENA, 0, l_speed);
    pca9685PWMWrite(fd, ENB, 0, r_speed);
}
void go_Advance(int fd,int l_speed,int r_speed){
    digitalWrite(IN1,LOW);
    digitalWrite(IN2,HIGH);
    digitalWrite(IN3,LOW);
    digitalWrite(IN4,HIGH); 
    pca9685PWMWrite(fd, ENA, 0, l_speed);
    pca9685PWMWrite(fd, ENB, 0, r_speed);
}
void go_Left(int fd,int l_speed,int r_speed){
    digitalWrite(IN1,HIGH);
    digitalWrite(IN2,LOW);
    digitalWrite(IN3,LOW);
    digitalWrite(IN4,HIGH); 
    pca9685PWMWrite(fd, ENA, 0, l_speed);
    pca9685PWMWrite(fd, ENB, 0, r_speed);
}
void go_Right(int fd,int l_speed,int r_speed){
    digitalWrite(IN1,LOW);
    digitalWrite(IN2,HIGH);
    digitalWrite(IN3,HIGH);
    digitalWrite(IN4,LOW); 
    pca9685PWMWrite(fd, ENA, 0, l_speed);
    pca9685PWMWrite(fd, ENB, 0, r_speed);
}

double measure(){
	double start = 0, stop = 0, elapsed = 0, distance = 0;
	
	digitalWrite(GPIO_TRIGGER, true);
	digitalWrite(GPIO_TRIGGER, false);
	start = clock();
	
	while (digitalRead(GPIO_ECHO) == 0)
	{
		start = clock();
	}
	
	while (digitalRead(GPIO_ECHO) == 1)
	{
		stop = clock();
	}
	
	elapsed = stop - start;
	distance = (elapsed * 34300) / 2;
	
	return distance;
}

// Function to halt program execution for a given number of milliseconds
void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

void stop_car(int fd){
    digitalWrite(IN1,LOW);
    digitalWrite(IN2,LOW);
    digitalWrite(IN3,LOW);
    digitalWrite(IN4,LOW); 
    pca9685PWMWrite(fd, ENA, 0, 0);
    pca9685PWMWrite(fd, ENB, 0, 0);
}
// ctrl-C key event handler
void my_handler(int s){
           stop_car( fd);
           printf("Ctrl C detected %d\n",s);
           exit(1); 

}

void die(char *s)
{
	perror(s);
	exit(1);
}

int main(void)
{
    //set up wiringPi GPIO 
    if(wiringPiSetup()==-1){
        printf("setup wiringPi failed!\n");
        printf("please check your setup\n");
        return -1;
    }

    //set up GPIO pin mode
	setup();
 
	// Setup PCA9685 with pinbase 300 and i2c location 0x40
	fd = pca9685Setup(PIN_BASE, 0x40, HERTZ);
	if (fd < 0)
	{
		printf("Error in setup\n");
		return fd;
	}

   	// following 5 lines define ctrl-C events
   	struct sigaction sigIntHandler;
   	sigIntHandler.sa_handler = my_handler;
   	sigemptyset(&sigIntHandler.sa_mask);
   	sigIntHandler.sa_flags = 0;
   	sigaction(SIGINT, &sigIntHandler, NULL);

    //following 20 lines set up Socket to receive UDP
	struct sockaddr_in si_me, si_other;
	int s, i, slen = sizeof(si_other) , recv_len;
	char buf[BUFLEN];
	//create a UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}
	
	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind socket to port
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		die("bind");
	}
	printf("Waiting for APP command...");
	//keep listening for data
	while(1)
	{
		
		fflush(stdout);
		
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			die("recvfrom()");
		}

 

    switch(buf[0])
    {
            case 'A':
            printf("move forward\n");
            go_Advance( fd,mid_speed,mid_speed);
            break;

            case 'B':
            printf("move backward\n");
            go_Back( fd,mid_speed,mid_speed);
            break;

            case 'L':
            printf("turn left\n");
            go_Left( fd,0,low_speed);
            break;

            case 'R':
            printf("turn right\n");
            go_Right( fd,low_speed,0);
            break;

            case 'E':
            printf("stop\n");
            stop_car( fd);
            break;
      }

		
		//print details of the client/peer and the data received
		//printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		//printf("Data: %c\n" , buf[0]);
		
		//now reply the client with the same data
        /*
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
		{
			die("sendto()");
		}
        */
		
		//if (OBSTACLE AVOIDANCE BUTTON TOGGLED){
		int sts1 = 0, sts2 = 0, sts3 = 0, ob_range = 30;
		
		//SET SERVO LEFT
		delay(300);
		distance = measure();
		if (distance > ob_range) { sts1 = 0; }
		else {sts1 = 1; }
		
		//SET SERVO CENTER
		delay(300);
		distance = measure();
		if (distance > ob_range) { sts2 = 0; }
		else {sts2 = 1; }
		
		//SET SERVO RIGHT
		delay(300);
		distance = measure();
		if (distance > ob_range) { sts3 = 0; }
		else {sts3 = 1; }
		
		if (sts1 == 1 && sts2 == 0 && sts3 == 0)
		{
			printf("100 slight right");
			go_Advance(fd, high_speed_2, 0);
			delay(long_delay);
			stop_car();
			delay(short_delay);
		}
		
		if (sts1 = 0 && sts2 == 0 && sts3 == 1)
		{
			printf("001 slight left");
			go_Advance(fd, 0, high_speed_2);
			delay(long_delay);
			stop_car();
			delay(short_delay);
		}
		
		if (sts1 == 1 && sts2 == 1 && sts3 == 0)
		{
			printf("110 sharp right");
			go_Right(fd, high_speed_2, low_speed_2);
			delay(long_delay);
			stop_car();
			delay(short_delay);
		}
		
		if ((sts1 == 0 && sts2 == 1 && sts3 == 1) || (sts1 == 0 && sts2 == 1 && sts3 == 0))
		{
			printf("%d%d%d sharp left", sts1, sts2, sts3);
			go_Left(fd, low_speed_2, high_speed_2);
			delay(long_delay);
			stop_car();
			delay(short_delay);
		}
		
		if ((sts1 == 1 && sts2 == 1 && sts3 == 1) || (sts1 == 1 & sts2 == 0 && sts3 == 1))
		{
			printf("%d%d%d back to left", sts1, sts2, sts3);
			go_Right(fd, high_speed_2, high_speed_2);
			delay(long_delay);
			stop_car();
			delay(short_delay);
		}
		
		if (sts1 == 0 && sts2 == 0 && sts3 == 0)
		{
			printf("000 forward");
			go_Advance(fd, mid_speed_2, mid_speed_2);
			delay(long_delay);
			stop_car();
			delay(short_delay);
		}
		//}
	}

	close(s);
	return 0;
}

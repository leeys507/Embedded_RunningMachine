#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>

#define LED_FILE_NAME "/dev/led_speed_driver"
#define SWITCH_FILE_NAME "/dev/switch_speed_driver"
#define SOUND_FILE_NAME "/dev/sound_sensor_driver"
#define ROTATE_FILE_NAME "/dev/rotate_driver"
#define MOTOR_FILE_NAME "/dev/motor_driver"
#define MATRIX_FILE_NAME "/dev/matrix_driver"
#define EXIT_FILE_NAME "/dev/exit_btn_driver"

void *readSwitch(void*);
void *readSound(void*);
void *writeRotate(void*);
void *writeMatrix(void*);
void *exit_speed(void*);
void fileOpen_ErrorCheck(int, char*);
int speed = 0;
int remain_angle;			// quit while angle up, down

typedef enum bool { false, true } bool;

bool angle = false;			// angle == false -> angle down
bool exit_btn = false;		// exit button

typedef struct driver_fd {
	int led_fd;			// speed led
	int switch_fd;		// switch button speed up, down
	int sound_fd;		// sound sensor
	int rotate_fd;		// 360 servo motor rotate
	int motor_fd;		// 180 servo motor angle
	int matrix_fd;		// dot matrix
} driver_fd;

int main(int argc, char ** argv) {
	int led_fd, switch_fd, sound_fd, rotate_fd, motor_fd, matrix_fd, exit_fd;
	int switchState, soundState, rotateState, matrixState, exit_speed_state;
	pthread_t switchThread, soundThread, rotateThread, matrixThread, exit_speed_thread;
	int exit_retval, r;
	char exit_check;
	bool exit_speed_thread_create = false;
	struct pollfd exit_events[2];

	led_fd = open(LED_FILE_NAME, O_RDWR);		// file open
	switch_fd = open(SWITCH_FILE_NAME, O_RDONLY | O_NONBLOCK);
	sound_fd = open(SOUND_FILE_NAME, O_RDONLY | O_NONBLOCK);
	rotate_fd = open(ROTATE_FILE_NAME, O_WRONLY);
	motor_fd = open(MOTOR_FILE_NAME, O_WRONLY);
	matrix_fd = open(MATRIX_FILE_NAME, O_WRONLY);
	exit_fd = open(EXIT_FILE_NAME, O_RDONLY | O_NONBLOCK);

	fileOpen_ErrorCheck(led_fd, LED_FILE_NAME);			// file open error check
	fileOpen_ErrorCheck(switch_fd, SWITCH_FILE_NAME);
	fileOpen_ErrorCheck(sound_fd, SOUND_FILE_NAME);
	fileOpen_ErrorCheck(rotate_fd, ROTATE_FILE_NAME);
	fileOpen_ErrorCheck(motor_fd, MOTOR_FILE_NAME);
	fileOpen_ErrorCheck(matrix_fd, MATRIX_FILE_NAME);
	fileOpen_ErrorCheck(exit_fd, EXIT_FILE_NAME);

	driver_fd *df = (driver_fd*)malloc(sizeof(driver_fd));		// thread argument
	df->led_fd = led_fd;			// allocate struct file descriptor
	df->switch_fd = switch_fd;
	df->sound_fd = sound_fd;
	df->rotate_fd = rotate_fd;
	df->motor_fd = motor_fd;
	df->matrix_fd = matrix_fd;

	switchState = pthread_create(&switchThread, NULL, readSwitch, (void*)df);		// create thread
	soundState = pthread_create(&soundThread, NULL, readSound, (void*)df);
	rotateState = pthread_create(&rotateThread, NULL, writeRotate, (void*)rotate_fd);
	matrixState = pthread_create(&matrixThread, NULL, writeMatrix, (void*)matrix_fd);

	if(switchState != 0 || soundState != 0 || rotateState != 0 || matrixState != 0) {		// check create thread error
		puts("thread create error");
		exit(1);
	}

	puts("<< Program Start >>");
	while(1) {		// exit button interrupt
		exit_events[0].fd = exit_fd;
		exit_events[0].events = POLLIN;

		exit_retval = poll(exit_events, 1, 10000);		// 10 sec timeout
		if(exit_retval < 0) {
			fprintf(stderr, "Poll error \n");
			exit(0);
		}
		else if(exit_retval == 0) {
			puts("Main");
		}
		else if(exit_events[0].revents & POLLIN) {		// if push exit button 
			read(exit_fd, &exit_check, sizeof(char));
			break;
		}
	}

	exit_btn = true;
	if(speed != 0) {
		exit_speed_state = pthread_create(&exit_speed_thread, NULL, exit_speed, NULL);
		if(exit_speed_state != 0) {
			puts("thread create error");
			exit(1);
		}
		exit_speed_thread_create = true;
	}

	if(angle) {			// if angle up
		int i;
		printf("<< wait for exit, because angle up >>\n");
		for(i = 0; i < remain_angle; i++)
			write(motor_fd, 0, sizeof(char));
	}
	else if(!angle && remain_angle != 0 && remain_angle != 20) {		// if angle down execute and exsist remain angle
		int i;
		printf("<< wait for exit, because angle down >>\n");
		for(i = remain_angle; i < 20; i++)
			write(motor_fd, 0, sizeof(char));
	}

	if(exit_speed_thread_create)		// if speed != 0 and thread created
		pthread_join(exit_speed_thread, (void*)&r);		// waiting for exit_speed_thread

	puts("<< Program Exit >>");

	free(df);		// memory return
	close(led_fd);		// close file
	close(switch_fd);
	close(sound_fd);
	close(rotate_fd);
	close(motor_fd);
	close(matrix_fd);
	close(exit_fd);

	return 0;
}

void *readSwitch(void *arg) {		// switch button interrupt
	driver_fd *df = (driver_fd*)arg;
	int switch_fd = df->switch_fd;
	int led_fd = df->led_fd;
	struct pollfd events[2];
	
	int retval;
	char data;
	
	while(1) {
		events[0].fd = switch_fd;
		events[0].events = POLLIN;

		retval = poll(events, 1, 10000);		// 10 sec timeout

		if(exit_btn) pthread_exit((void*)&retval);			// if exit button push, thread exit

		if(retval < 0) {
			fprintf(stderr, "Poll error \n");
			exit(0);
		}
		else if(retval == 0) {
			puts("Switch no action");
		}
		else if(events[0].revents & POLLIN) {
			read(switch_fd, &data, sizeof(char));
			
			if(data == 1) {			// push speed up switch
				if(speed < 3)
					speed += 1;
				printf("switch execute %d\n", speed);
				write(led_fd, &speed, sizeof(char));
				usleep(600 * 1000);
			}
			else if(data == 2) {		// push speed down switch
				if(speed > 0)
					speed -= 1;
				printf("switch execute %d\n", speed);
				write(led_fd, &speed, sizeof(char));
				usleep(600 * 1000);
			}
			read(switch_fd, &data, sizeof(char));		// block double push
		}
	}
}

void *readSound(void *arg) {		// sound sensor interrupt
	driver_fd *df = (driver_fd*)arg;
	int sound_fd = df->sound_fd;
	int motor_fd = df->motor_fd;
	struct pollfd events[2], events2[2];

	int retval;
	char data;
	while(1) {
		events[0].fd = sound_fd;
		events[0].events = POLLIN;

		retval = poll(events, 1, 10000);
		
		if(exit_btn) pthread_exit((void*)retval);

		if(retval < 0) {
			fprintf(stderr, "Poll error \n");
			exit(0);
		}
		else if(retval == 0) {
			puts("Sound not detection");
		}
		else if(events[0].revents & POLLIN) { 		// sound cnt 1
			usleep(500 * 1000);
			read(sound_fd, &data, sizeof(char));	// block double read
			usleep(500 * 1000);

			events2[0].fd = sound_fd;
			events2[0].events = POLLIN;
			retval = poll(events2, 1, 500);

			if(retval < 0) {
				fprintf(stderr, "Poll error \n");
				exit(0);
			}
			else if((events2[0].revents & POLLIN)) {		// sound cnt 2
				read(sound_fd, &data, sizeof(char));
				if(data == 1) {
					if(!angle) {
						angle = true;		// up
						printf("<< angle up excute >>\n");
						for(remain_angle = 0; remain_angle < 20; remain_angle++) {
							if(!exit_btn)
								write(motor_fd, &data, sizeof(char));		// 180 servo angle up
							else break;
						}
						usleep(600 * 1000);
					}
					else {
						angle = false;		// down
						printf("<< angle down excute >>\n");
						data = 0;

						bool printExitMsg = false;
						for(remain_angle = 0; remain_angle < 20; remain_angle++) {
							if(!exit_btn)
								write(motor_fd, &data, sizeof(char));		// 180 servo angle down
							else break;
						}
						usleep(600 * 1000);
					}
					if(exit_btn) break;
					else printf("<< angle excute finish >>\n");
				}
			}
		}
	}	
}

void *writeRotate(void *arg) {
	int rotate_fd = (int)arg;

	while(1) {
		write(rotate_fd, &speed, sizeof(char));
	}
}

void *writeMatrix(void *arg) {
	int matrix_fd = (int)arg;
	int cnt, r;
	int runningTime = 0;
	int sec_index, i, j;
	bool speed_check = false;
	char num[11][5] = {
		0xc3, 0xdb, 0xdb, 0xdb, 0xc3,	// 0
		0xf7, 0xe7, 0xf7, 0xf7, 0xe3,	// 1
		0xc3, 0xfb, 0xc3, 0xdf, 0xc3,	// 2
		0xc3, 0xfb, 0xc3, 0xfb, 0xc3,	// 3
		0xd7, 0xd7, 0xc3, 0xf7, 0xf7,	// 4
		0xc3, 0xdf, 0xc3, 0xfb, 0xc3,	// 5
		0xdf, 0xdf, 0xc3, 0xdb, 0xc3,	// 6
		0xc3, 0xdb, 0xdb, 0xfb, 0xfb,	// 7
		0xc3, 0xdb, 0xc3, 0xdb, 0xc3,	// 8
		0xc3, 0xdb, 0xc3, 0xfb, 0xfb,	// 9
		0xa3, 0xab, 0xab, 0xab, 0xa3	// 10
	};

	while(1) {
		for(i = 0; i < sizeof(num) / sizeof(num[0]); i++) {		// all clear 10 sec dot point
			for(j = 0; j < sizeof(num[0]) / sizeof(char); j++) {
				num[i][j] |= 0x01;
			}
		}

		while(1) {
			if(speed == 0)
				write(matrix_fd, num[0], sizeof(char));		// write 1 sec
			else {
				speed_check = false;
				break;
			}
		}

		while(1) {
			printf("<< runningTime = %d minute >>\n", runningTime);
			for(cnt = 0; cnt < 60; cnt++) {				// 60 sec
				if(cnt != 0 && (cnt % 10 == 0)) {		// show 10 sec dot point
					sec_index = cnt / 10;
					num[runningTime % 10][sec_index - 1] -= 1;
				}
				
				if(exit_btn) pthread_exit((void*)r);

				if(speed != 0)
					write(matrix_fd, num[runningTime % 10], sizeof(char));
				else {
					speed_check = true;
					break;
				}
			}
			if(speed_check == true) {
				runningTime = 0;
				break;
			}
			for(i = 0; i < 5; i++)		// clear 10 sec dot point
				num[runningTime % 10][i] += 1; 
			runningTime++;
		}
	}
}

void *exit_speed(void *arg) {		// if push exit button and speed != 0, slow down
	switch(speed) {
		case 1:
			speed = 0;
			break;
		case 2:
			speed = 1;
			sleep(2);
			speed = 0;
			break;
		case 3:
			speed = 2;
			sleep(2);
			speed = 1;
			sleep(2);
			speed = 0;
			break;
	}
}

void fileOpen_ErrorCheck(int fd, char *file_name) {	
	if(fd < 0) {
		fprintf(stderr, "Can't open %s\n", file_name);
		exit(1);
	}
}

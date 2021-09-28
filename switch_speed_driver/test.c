#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/poll.h>
#include <signal.h>
#include <pthread.h>

#define LED_FILE_NAME "/dev/led_speed_driver"
#define SWITCH_FILE_NAME "/dev/switch_speed_driver"
#define SOUND_FILE_NAME "/dev/sound_sensor_driver"

void *readSwitch(void*);
void *readSound(void*);
int speed = 0;

typedef struct driver_fd {
	int led_fd;
	int switch_fd;
	int sound_fd;
} driver_fd;

int main(int argc, char ** argv) {
	int led_fd, switch_fd, sound_fd;
	int switchState, soundState;
	pthread_t readSwitchID, readSoundID;

	led_fd = open(LED_FILE_NAME, O_RDWR);
	switch_fd = open(SWITCH_FILE_NAME, O_RDONLY | O_NONBLOCK);
	sound_fd = open(SOUND_FILE_NAME, O_RDONLY);

	if(led_fd < 0 || switch_fd < 0 || sound_fd < 0) {
		fprintf(stderr, "Can't open %s or %s or %s\n", LED_FILE_NAME, SWITCH_FILE_NAME, SOUND_FILE_NAME);
		return -1;
	}
	driver_fd *df = (driver_fd*)malloc(sizeof(driver_fd));
	df->led_fd = led_fd;
	df->switch_fd = switch_fd;
	df->sound_fd = sound_fd;

	switchState = pthread_create(&readSwitchID, NULL, readSwitch, (void*)df);
	soundState = pthread_create(&readSoundID, NULL, readSound, (void*)df);
	if(switchState != 0 || soundState != 0) {
		puts("thread create error");
		exit(1);
	}

	while(1) {
		puts("Main");
		sleep(1);
	}

	close(led_fd);
	close(switch_fd);
	close(sound_fd);
	return 0;
}

void *readSwitch(void *arg) {
	driver_fd *df = (driver_fd*)arg;
	int switch_fd = df->switch_fd;
	int led_fd = df->led_fd;
	struct pollfd events[2];
	
	int retval;
	char data;
	
	while(1) {
		events[0].fd = switch_fd;
		events[0].events = POLLIN;

		retval = poll(events, 1, 1000);
		if(retval < 0) {
			fprintf(stderr, "Poll error \n");
			exit(0);
		}
			
		if(events[0].revents & POLLIN) {
			read(switch_fd, &data, sizeof(char));
			if(data == 1) {
				if(speed < 3)
					speed += 1;
				printf("switch execute %d\n", speed);
				write(led_fd, &speed, sizeof(char));
				usleep(200 * 1000);
			}
			else if(data == 2) {
				if(speed > 0)
					speed -= 1;
				printf("switch execute %d\n", speed);
				write(led_fd, &speed, sizeof(char));
				usleep(200 * 1000);
			}
		}			
	}
}

void *readSound(void *arg) {
	driver_fd *df = (driver_fd*)arg;
	int sound_fd = df->sound_fd;
	int led_fd = df->led_fd;
	char data;
	while(1) {
		read(sound_fd, &data, sizeof(char));
		printf("data: %d \n", data);
		if(data == 1) {
			if(speed < 3)
				speed += 1;
			else
				speed = 0;
			printf("sound execute %d\n", speed);
			write(led_fd, &speed, sizeof(char));
			usleep(200 * 1000);
		}
		else usleep(200 * 1000);
	}	
}

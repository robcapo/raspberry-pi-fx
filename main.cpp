#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <jack/jack.h>
#include "delay_buffer.cpp"
#include "fx_processor.cpp"


#define SAMPLE_RATE 44100
#define DURATION_SECONDS 1

Delay_Buffer buf;
FX_Processor fx(NONE);

jack_port_t *input_port;
jack_port_t *output_port;

void *uartThread(void *arg);

/** This function is called every time a frame of samples becomes available.
 *
 * The process function passes the incoming frame of samples to the delay buffer. Once the
 * delay class's output buffer is ready, the process function copies the output buffer to
 * the output sound buffer.
 *
 * @param nframes The number of samples in the current frame.
 */
int process (jack_nframes_t nframes, void *arg)
{
    jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer(output_port, nframes);
    jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer(input_port, nframes);

	buf.newFrame(in);
	
	memcpy(out, buf._output_buffer, sizeof(jack_default_audio_sample_t) * nframes);

	return 0;
}

void jack_shutdown(void *arg)
{
    printf("Jack Shutdown");
    exit(1);
}

/** Prepares jack client and creates thread to read UART.
 *
 * The main function sets up the JACK client, creates the FX processor object and the
 * delay buffer object, and creates a separate thread to constantly read the serial port
 * where UART info is being sent from the Tiva C.
 */
int main(int argc, const char * argv[])
{
	jack_client_t *client;
	const char **ports;

	client = jack_client_open("client", JackNullOption, NULL);

	fx = FX_Processor(NONE);
	fx.setParam(TR_RATE, 0.1);
	fx.setParam(TR_OFF_VOLUME, .1);
	fx.setParam(DS_DIST, 1);
	fx.setParam(WAH_DURATION, 1);

	buf = Delay_Buffer(.6, 1, 1, jack_get_buffer_size(client));
	buf._fx_processor = &fx;
	

	jack_set_process_callback(client, process, 0);
	jack_on_shutdown(client, jack_shutdown, 0);

	input_port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	output_port = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	if (jack_activate(client)) {
		printf("Could not activate client");
		exit(1);
	}
	//*
	if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsOutput | JackPortIsPhysical)) == NULL) {
		printf("Could not find input ports\n");
		exit(1);
	}

	printf("Connecting %s to %s\n", ports[0], jack_port_name(input_port));
 

	if (jack_connect(client, ports[0], jack_port_name(input_port))) {
		printf("Could not connect to input ports\n");
		exit(1);
	}
	//*/
	//*
	if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsInput | JackPortIsPhysical)) == NULL) {
		printf("Could not find output ports\n");
		exit(1);
	}



	printf("Connecting %s to %s\n", jack_port_name(output_port), ports[0]);

	if (jack_connect(client, jack_port_name(output_port), ports[0])) {
		printf("Could not connect to output ports\n");
		exit(1);
	}
	//*/

	free(ports);

	pthread_t pth;
	pthread_create(&pth, NULL, uartThread, 0);
	
	sleep(100000);
	
	jack_client_close(client);

	return 0;
}

/** Continually reads the serial port where UART is being sent from the Tiva C.
 *
 * This function opens the ttyAMA0 device, which is the serial port where UART is
 * connected, and reads it in a while loop on a separate thread from the JACK client. The
 * Tiva C can send messages to cycle through the FX or to change the tempo of the delay.
 */
void *uartThread(void *arg)
{
     fd = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY);
     if (fd == -1)
     {
        perror("open_port: Unable to open /dev/ttyO1\n");
        exit(1);
     }
	struct termios options;
	tcgetattr(fd, &options);
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);	

	char uart_buffer[256];
	while (1) {
		int n = read(fd, uart_buffer, 256);
		if (n == -1) {
			perror("Error reading file\n");
		} else if (n > 0) {
			if (uart_buffer[0] == 'a') {
				// cycle through fx
				fx.nextFx();
			} else {
				double tempo = 0;
				for (int i = 0; i < strlen(uart_buffer) - 2; i++) {
					if (uart_buffer[i] >= '0' && uart_buffer[i] <= '9') {
						 tempo = tempo * 10 + (uart_buffer[i] - '0');
					}
				}
				
				tempo = tempo / 1000;
				printf("Tempo: %f", tempo);
				buf.setDelayLength(tempo);
			}
		}
	}
	
}
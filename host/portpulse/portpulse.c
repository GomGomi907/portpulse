/* PortPulse native CDC host utility. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

static void usage(const char *name) {
    fprintf(stderr, "Usage: %s --port <COMx|/dev/ttyACMx> <on|off|cycle|status|fault|version|help> [--delay-ms N]\n", name);
}

#ifdef _WIN32
static HANDLE serial_open(const char *path) {
    char device[64];
    snprintf(device, sizeof(device), "\\\\.\\%s", path);
    HANDLE h = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return h;
    DCB dcb = {0}; dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }
    dcb.BaudRate = CBR_115200; dcb.ByteSize = 8; dcb.StopBits = ONESTOPBIT; dcb.Parity = NOPARITY;
    dcb.fDtrControl = DTR_CONTROL_DISABLE; dcb.fRtsControl = RTS_CONTROL_DISABLE;
    if (!SetCommState(h, &dcb)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }
    COMMTIMEOUTS t = {0}; t.ReadIntervalTimeout = 50; t.ReadTotalTimeoutConstant = 1000; t.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &t); return h;
}
static int serial_exchange(HANDLE h, const char *cmd, char *reply, size_t cap) {
    char line[128]; DWORD written = 0; snprintf(line, sizeof(line), "%s\n", cmd);
    if (!WriteFile(h, line, (DWORD)strlen(line), &written, NULL)) return 0;
    DWORD n = 0; size_t used = 0;
    while (used + 1 < cap && ReadFile(h, reply + used, 1, &n, NULL) && n) { if (reply[used++] == '\n') break; }
    reply[used] = '\0'; return used != 0;
}
#else
static int serial_open(const char *path) {
    int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC); if (fd < 0) return -1;
    struct termios tty; if (tcgetattr(fd, &tty) != 0) { close(fd); return -1; }
    cfsetospeed(&tty, B115200); cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; tty.c_cflag |= CLOCAL | CREAD; tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_iflag = 0; tty.c_oflag = 0; tty.c_lflag = 0; tty.c_cc[VMIN] = 0; tty.c_cc[VTIME] = 10;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return -1; } return fd;
}
static int serial_exchange(int fd, const char *cmd, char *reply, size_t cap) {
    char line[128]; snprintf(line, sizeof(line), "%s\n", cmd); if (write(fd, line, strlen(line)) < 0) return 0;
    size_t used = 0; while (used + 1 < cap) { char c; ssize_t n = read(fd, &c, 1); if (n <= 0) break; reply[used++] = c; if (c == '\n') break; }
    reply[used] = '\0'; return used != 0;
}
#endif

int main(int argc, char **argv) {
    const char *port = NULL; const char *command = NULL; int delay = 2000; char request[128];
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--port") && i + 1 < argc) port = argv[++i];
        else if (!strcmp(argv[i], "--delay-ms") && i + 1 < argc) delay = atoi(argv[++i]);
        else if (!command) command = argv[i]; else { usage(argv[0]); return 2; }
    }
    if (!port || !command) { usage(argv[0]); return 2; }
    if (!strcmp(command, "cycle")) snprintf(request, sizeof(request), "cycle %d", delay); else snprintf(request, sizeof(request), "%s", command);
#ifdef _WIN32
    HANDLE serial = serial_open(port); if (serial == INVALID_HANDLE_VALUE) { fprintf(stderr, "cannot open %s\n", port); return 3; }
#else
    int serial = serial_open(port); if (serial < 0) { fprintf(stderr, "cannot open %s: %s\n", port, strerror(errno)); return 3; }
#endif
    char reply[256]; int ok = serial_exchange(serial, request, reply, sizeof(reply));
#ifdef _WIN32
    CloseHandle(serial);
#else
    close(serial);
#endif
    if (!ok) { fprintf(stderr, "no response from PortPulse\n"); return 4; }
    fputs(reply, stdout); return strncmp(reply, "ERR", 3) == 0 ? 1 : 0;
}

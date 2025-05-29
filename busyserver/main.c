#include "stdio.h"
#include "unistd.h"
#include "time.h"

void hoonow(char * buf, size_t size) {
    time_t now = time(NULL);
    struct tm * timeinfo;
    timeinfo = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}
void hoolog(char * msg) {
    char timebuf[20];
    hoonow(timebuf, sizeof(timebuf));
    printf("[%s] %s\n", timebuf, msg);
}
void hoosleep(int seconds) {
	sleep(seconds);
}
void hoousleep(int ms) {
	usleep(ms);
}
int main() {
    hoolog("Server startup.");
    return 0;
}

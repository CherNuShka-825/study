#include <unistd.h>
#include <sys/syscall.h>

ssize_t my_write(int fd, const void *buf, size_t count) {
    return syscall(SYS_write, fd, buf, count);
}

int main(void) {
    my_write(1, "Hello world\n", 12);
    return 0;
}


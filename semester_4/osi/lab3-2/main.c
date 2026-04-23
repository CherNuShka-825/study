#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024


static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void cmd_unlink_path(const char *path) {
    if (unlink(path) == -1) {
        die("unlink");
    }
}

static void cmd_mk_dir(const char *path) {
    if (mkdir(path, 0777) == -1) {
        die("mkdir");
    }
}

static void cmd_ls_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        die("opendir");
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    if (closedir(dir) == -1) {
        die("closedir");
    }
}

static void cmd_rm_dir(const char *path) {
    //отличие
    if (rmdir(path) == -1) {
        die("rmdir");
    }
}

static void cmd_mk_file(const char *path) {
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0666); //создать, если нет; ошибка, если существует; открыть только для записи
    if (fd == -1) {
        die("open");
    }
    if (close(fd) == -1) {
        die("close");
    }
}

static void cmd_cat_file(const char *path) {
    //только ли опеном можно
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        die("open");
    }
    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w == -1) {
                close(fd);
                die("write");
            }
            written += w;
        }
    }
    if (n == -1) {
        close(fd);
        die("read");
    }
    if (close(fd) == -1) {
        die("close");
    }
}

static void cmd_rm_file(const char *path) {
    cmd_unlink_path(path);
}

static void cmd_mk_symlink(const char *target, const char *linkpath) {
    if (symlink(target, linkpath) == -1) {
        die("symlink");
    }
}

static void cmd_read_symlink(const char *linkpath) {
    char buf[BUF_SIZE];
    ssize_t n = readlink(linkpath, buf, sizeof(buf) - 1);
    if (n == -1) {
        die("readlink");
    }
    buf[n] = '\0';
    printf("%s\n", buf);
}

static void cmd_cat_symlink_target(const char *linkpath) {
    cmd_cat_file(linkpath);
}

static void cmd_rm_symlink(const char *path) {
    cmd_unlink_path(path);
}

static void cmd_mk_hardlink(const char *target, const char *linkpath) {
    if (link(target, linkpath) == -1) {
        die("link");
    }
}

static void cmd_rm_hardlink(const char *path) {
    cmd_unlink_path(path);
}

static void cmd_stat_file(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        die("stat");
    }
    printf("%o\n", st.st_mode & 0777); //тип+права
    printf("%lu\n", (unsigned long)st.st_nlink);
}

static void cmd_chmod_file(const char *mode_str, const char *path) {
    char *endptr;
    long mode = strtol(mode_str, &endptr, 8);
    if (*endptr != '\0') {
        exit(1); //не die, ибо не системная ошибка
    }
    if (chmod(path, (mode_t)mode) == -1) {
        die("chmod");
    }
}

static void usage() {
    printf("Usage:\n");
    printf("  mk_dir <path>\n");
    printf("  ls_dir <path>\n");
    printf("  rm_dir <path>\n");
    printf("  mk_file <path>\n");
    printf("  cat_file <path>\n");
    printf("  rm_file <path>\n");
    printf("  mk_symlink <target> <link>\n");
    printf("  read_symlink <link>\n");
    printf("  cat_symlink_target <link>\n");
    printf("  rm_symlink <link>\n");
    printf("  mk_hardlink <target> <link>\n");
    printf("  rm_hardlink <link>\n");
    printf("  stat_file <path>\n");
    printf("  chmod_file <mode> <path>\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "mk_dir") == 0) {
        if (argc != 3) return 1;
        cmd_mk_dir(argv[2]);
    }
    else if (strcmp(cmd, "ls_dir") == 0) {
        if (argc != 3) return 1;
        cmd_ls_dir(argv[2]);
    }
    else if (strcmp(cmd, "rm_dir") == 0) {
        if (argc != 3) return 1;
        cmd_rm_dir(argv[2]);
    }
    else if (strcmp(cmd, "mk_file") == 0) {
        if (argc != 3) return 1;
        cmd_mk_file(argv[2]);
    }
    else if (strcmp(cmd, "cat_file") == 0) {
        if (argc != 3) return 1;
        cmd_cat_file(argv[2]);
    }
    else if (strcmp(cmd, "rm_file") == 0) {
        if (argc != 3) return 1;
        cmd_rm_file(argv[2]);
    }
    else if (strcmp(cmd, "mk_symlink") == 0) {
        if (argc != 4) return 1;
        cmd_mk_symlink(argv[2], argv[3]);
    }
    else if (strcmp(cmd, "read_symlink") == 0) {
        if (argc != 3) return 1;
        cmd_read_symlink(argv[2]);
    }
    else if (strcmp(cmd, "cat_symlink_target") == 0) {
        if (argc != 3) return 1;
        cmd_cat_symlink_target(argv[2]);
    }
    else if (strcmp(cmd, "rm_symlink") == 0) {
        if (argc != 3) return 1;
        cmd_rm_symlink(argv[2]);
    }
    else if (strcmp(cmd, "mk_hardlink") == 0) {
        if (argc != 4) return 1;
        cmd_mk_hardlink(argv[2], argv[3]);
    }
    else if (strcmp(cmd, "rm_hardlink") == 0) {
        if (argc != 3) return 1;
        cmd_rm_hardlink(argv[2]);
    }
    else if (strcmp(cmd, "stat_file") == 0) {
        if (argc != 3) return 1;
        cmd_stat_file(argv[2]);
    }
    else if (strcmp(cmd, "chmod_file") == 0) {
        if (argc != 4) return 1;
        cmd_chmod_file(argv[2], argv[3]);
    }
    else {
        usage();
        return 1;
    }

    return 0;
}
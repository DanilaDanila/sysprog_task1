#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fanotify.h>
#include <unistd.h>

static void handle_events(int fd, char *abs_path_to_subdir) {
    const struct fanotify_event_metadata *metadata;
    struct fanotify_event_metadata buf[200];

    ssize_t len;
    char path[PATH_MAX];

    ssize_t path_len;
    char procfd_path[PATH_MAX];

    struct fanotify_response response;

    for (;;) {
        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            perror("[ERROR] read");
            exit(1);
        }

        if (len <= 0) {
            break;
        }

        metadata = buf;

        while (FAN_EVENT_OK(metadata, len)) {
            if (metadata->vers != FANOTIFY_METADATA_VERSION) {
                dprintf(STDERR_FILENO, "[ERROR] Mismatch of fanotify metadata version\n");
                exit(2);
            }

            if (metadata->fd >= 0) {
                // if (metadata->mask & FAN_MODIFY) {
                //     printf("FAN_MODIFY: ");
                // }

                snprintf(procfd_path, sizeof(procfd_path), "/proc/self/fd/%d", metadata->fd);
                path_len = readlink(procfd_path, path, sizeof(path) - 1);

                if (path_len == -1) {
                    perror("[ERROR] readlink");
                    exit(3);
                }

                path[path_len] = '\0';

                // README.md --> Notes --> 1
                if (strstr(path, abs_path_to_subdir) == path) {
                    printf("[NOTIFY] File modified: '%s'\n", path);
                } else {
                    // nop
                }

                close(metadata->fd);
            }

            metadata = FAN_EVENT_NEXT(metadata, len);
        }
    }
}

int get_fanotify_modify_fd(char *mp_abs_path) {
    int fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);

    if (fd == -1) {
        perror("[ERROR] fanotify init");
        exit(5);
    }

    if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_MODIFY, 0 /*absolute path only*/, mp_abs_path) == -1) {
        perror("[ERROR] fanotify_mark");
        exit(6);
    }

    return fd;
}

int get_fanotify_delete_self_fd(char *mp_abs_path) {
    int fd = fanotify_init(FAN_REPORT_FID | FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);

    if (fd == -1) {
        perror("[ERROR] delete_self: fanotify init");
        exit(5);
    }

    if (fanotify_mark(fd, FAN_MARK_ADD, FAN_DELETE_SELF | FAN_ONDIR, 0 /*absolute path only*/, mp_abs_path) == -1) {
        perror("[ERROR] delete_self: fanotify_mark");
        exit(6);
    }

    return fd;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <mount point>\n", argv[0]);
        exit(4);
    }

    char buf;
    int poll_num;

    int fa_modify_fd = get_fanotify_modify_fd(argv[1]);
    int fa_delete_self_fd = get_fanotify_delete_self_fd(argv[1]);


    nfds_t nfds = 3;
    struct pollfd fds[3];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = fa_modify_fd;
    fds[1].events = POLLIN;

    fds[2].fd = fa_delete_self_fd;
    fds[2].events = POLLIN;

    printf("[LOG] Press eny key to terminate\n");
    printf("[LOG] Listening for events\n");

    for (;;) {
        poll_num = poll(fds, nfds, -1);

        if (poll_num == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("[ERROR] poll");
            exit(7);
        }

        if (poll_num > 0) {
            if (fds[0].revents & POLLIN) {
                while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n') {
                    continue;
                }

                break;
            }

            /* modify */
            if (fds[1].revents & POLLIN) {
                handle_events(fa_modify_fd, argv[1]);
            }

            /* delete_self */
            if (fds[2].revents & POLLIN) {
                printf("[LOG] Whatched folder had been deleted!\n[LOG] Shutting down...\n");
                break;
            }
        }
    }

    printf("[LOG] Listening for events stopped.\n");
    exit(0);
}

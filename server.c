#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "protocol.h"

#define MAX_BOOKINGS 100
#define MAX_USERS 100

#define FILE_USERS "users.txt"
#define FILE_BOOKINGS "bookings.txt"

#define ADMIN_USER "admin"
#define ADMIN_PASSWORD "admin"

typedef struct {
    int id;
    char resource[50];
    char date[20];
    char time_start[10];
    char time_end[10];
    char requester[50];
    char status[20]; // pending, approved, refused
} Booking;

typedef struct {
    int id;
    char username[50];
    char password[50];
} User;

Booking bookings[MAX_BOOKINGS];
int num_bookings = 0;
int next_booking_id = 1;

User users[MAX_USERS];
int num_users = 0;
int next_user_id = 1;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// UTILITY FUNCTIONS

int valid_date(char *date) {
    int d, m, y;
    if (sscanf(date, "%d/%d/%d", &d, &m, &y) != 3) {
        return 0;
    }
    return d >= 1 && d <= 31 && m >= 1 && m <= 12 && y >= 2026;
}

int valid_hour(char *hour) {
    int h, m;
    if (sscanf(hour, "%d:%d", &h, &m) != 2) {
        return 0;
    }
    return h >= 0 && h <= 23 && m >= 0 && m <= 59;
}

int hour_to_minutes(char *hour) {
    int h, m;
    sscanf(hour, "%d:%d", &h, &m);
    return h * 60 + m;
}

int check_conflicts(Booking *a, Booking *b) {
    if (strcmp(a->resource, b->resource) != 0) return 0;
    if (strcmp(a->date, b->date) != 0) return 0;
    return hour_to_minutes(a->time_start) < hour_to_minutes(b->time_end) &&
           hour_to_minutes(b->time_start) < hour_to_minutes(a->time_end);
}

void append_response(char *dest, char *text) {
    size_t len = strlen(dest);
    size_t end_len = strlen(MSG_END);
    if (len < RESPONSE - end_len) snprintf(dest + len, RESPONSE - end_len - len, "%s", text);
}

void add_booking_row(char *dest, Booking *b) {
    char row[256];
    snprintf(row, sizeof(row), "[%d] %s | %s | %s-%s | requester: %s | %s\n", b->id, b->resource, b->date, b->time_start, b->time_end, b->requester, b->status);
    append_response(dest, row);
}

// FILE MANAGEMENT

void save_bookings() {
    FILE *f = fopen(FILE_BOOKINGS, "w");
    if (f == NULL) {
        perror("fopen bookings");
        return;
    }
    for (int i = 0; i < num_bookings; i++) {
        Booking *b = &bookings[i];
        fprintf(f, "%d|%s|%s|%s|%s|%s|%s\n", b->id, b->resource, b->date, b->time_start, b->time_end, b->requester, b->status);
    }
    fclose(f);
}

void save_users () {
    FILE *f = fopen(FILE_USERS, "w");
    if (f == NULL) {
        perror("fopen users");
        return;
    }
    for (int i = 0; i < num_users; i++) {
        User *u = &users[i];
        fprintf(f, "%d|%s|%s\n", u->id, u->username, u->password);
    }
    fclose(f);
}

void load_data() {
    FILE *f;

    f = fopen(FILE_USERS, "r");
    if (f != NULL) {
        char line[256];
        while (num_users < MAX_USERS && fgets(line, sizeof(line), f) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            User *u = &users[num_users];

            char *id_str = strtok(line, "|");
            char *username = strtok(NULL, "|");
            char *password = strtok(NULL, "|");

            if (id_str == NULL || username == NULL || password == NULL) {
                continue;
            }

            u->id = atoi(id_str);
            strncpy(u->username, username, sizeof(u->username) - 1);
            u->username[sizeof(u->username) - 1] = '\0';
            strncpy(u->password, password, sizeof(u->password) - 1);
            u->password[sizeof(u->password) - 1] = '\0';

            if (u->id >= next_user_id) {
                next_user_id = u->id + 1;
            }
            num_users++;
        }
        fclose(f);
    }

    f = fopen(FILE_BOOKINGS, "r");
    if (f != NULL) {
        char line[256];
        while (num_bookings < MAX_BOOKINGS && fgets(line, sizeof(line), f) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            Booking *b = &bookings[num_bookings];

            char *id_str = strtok(line, "|");
            char *resource = strtok(NULL, "|");
            char *date = strtok(NULL, "|");
            char *start = strtok(NULL, "|");
            char *end = strtok(NULL, "|");
            char *requester = strtok(NULL, "|");
            char *status = strtok(NULL, "|");

            if (id_str == NULL || resource == NULL || date == NULL || start == NULL || end == NULL || requester == NULL || status == NULL) {
                continue;
            }

            b->id = atoi(id_str);
            strncpy(b->resource, resource, sizeof(b->resource) - 1);
            b->resource[sizeof(b->resource) - 1] = '\0';
            strncpy(b->date, date, sizeof(b->date) - 1);
            b->date[sizeof(b->date) - 1] = '\0';
            strncpy(b->time_start, start, sizeof(b->time_start) - 1);
            b->time_start[sizeof(b->time_start) - 1] = '\0';
            strncpy(b->time_end, end, sizeof(b->time_end) - 1);
            b->time_end[sizeof(b->time_end) - 1] = '\0';
            strncpy(b->requester, requester, sizeof(b->requester) - 1);
            b->requester[sizeof(b->requester) - 1] = '\0';
            strncpy(b->status, status, sizeof(b->status) - 1);
            b->status[sizeof(b->status) - 1] = '\0';

            if (b->id >= next_booking_id) {
                next_booking_id = b->id + 1;
            }
            num_bookings++;
        }
        fclose(f);
    }

    printf("Loaded %d users & %d bookings\n", num_users, num_bookings);
}

// COMMANDS HANDLING

void cmd_register(char *response, char *user, char *pass) {
    if (user == NULL || pass == NULL) {
        sprintf(response, "ERROR: syntax not valid (%s|username|password)\n", CMD_REGISTER);
        return;
    }
    pthread_mutex_lock(&mutex);
    if (strcmp(user, ADMIN_USER) == 0) {
        snprintf(response, RESPONSE, "ERROR: username not available\n");
    } else if (num_users >= MAX_USERS) {
        snprintf(response, RESPONSE, "ERROR: user archive is full\n");
    } else {
        int exists = 0;
        for (int i = 0; i < num_users; i++) {
            if (strcmp(users[i].username, user) == 0) {
                exists = 1;
                break;
            }
        }
        if (exists) {
            snprintf(response, RESPONSE, "ERROR: user already registered\n");
        } else {
            users[num_users].id = next_user_id;
            strncpy(users[num_users].username, user, 49);
            users[num_users].username[49] = '\0';
            strncpy(users[num_users].password, pass, 49);
            users[num_users].password[49] = '\0';
            num_users++;
            save_users();
            next_user_id++;
            snprintf(response, RESPONSE, "OK: registration ended, now you can login\n");
        }
    }
    pthread_mutex_unlock(&mutex);
}

void cmd_login(char *response, char *user, char *pass, char *logged_user, int *admin) {
    if (user == NULL || pass == NULL) {
        snprintf(response, RESPONSE, "ERROR: syntax not valid (%s|username|password)\n", CMD_LOGIN);
        return;
    }
    if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASSWORD) == 0) {
        strncpy(logged_user, user, 49);
        logged_user[49] = '\0';
        *admin = 1;
        snprintf(response, RESPONSE, "%s: welcome\n", RESP_OK_ADMIN);
        return;
    }
    int found = 0;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, user) == 0 && strcmp(users[i].password, pass) == 0) {
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    if (found) {
        strncpy(logged_user, user, 49);
        logged_user[49] = '\0';
        *admin = 0;
        snprintf(response, RESPONSE, "%s: welcome %s\n", RESP_OK_USER, user);
    } else {
        snprintf(response, RESPONSE, "ERROR: wrong credentials (access not authorized)\n");
    }
}

void cmd_book(char *response, char *user, char *resource, char *date, char *start, char *end) {
    if (resource == NULL || date == NULL || start == NULL || end == NULL) {
        snprintf(response, RESPONSE, "ERROR: syntax not valid (%s|resource|date|start|end)\n", CMD_BOOK);
        return;
    }
    if (!valid_date(date)) {
        snprintf(response, RESPONSE, "ERROR: date is not valid, use dd/mm/yyyy\n");
        return;
    }
    if (!valid_hour(start) || !valid_hour(end)) {
        snprintf(response, RESPONSE, "ERROR: time is not valid, use hh:mm\n");
        return;
    }
    if (hour_to_minutes(start) >= hour_to_minutes(end)) {
        snprintf(response, RESPONSE, "ERROR: start time must be lower than end time\n");
        return;
    }

    pthread_mutex_lock(&mutex);

    if (num_bookings >= MAX_BOOKINGS) {
        snprintf(response, RESPONSE, "ERROR: booking archive is full\n");
        pthread_mutex_unlock(&mutex);
        return;
    }

    Booking new_booking;
    new_booking.id = next_booking_id;
    strncpy(new_booking.resource, resource, 49);
    new_booking.resource[49] = '\0';
    strncpy(new_booking.date, date, 19);
    new_booking.date[19] = '\0';
    strncpy(new_booking.time_start, start, 9);
    new_booking.time_start[9] = '\0';
    strncpy(new_booking.time_end, end, 9);
    new_booking.time_end[9] = '\0';
    strncpy(new_booking.requester, user, 49);
    new_booking.requester[49] = '\0';
    strcpy(new_booking.status, "pending");

    for (int i = 0; i < num_bookings; i++) {
        if (strcmp(bookings[i].status, "approved") == 0 && check_conflicts(&new_booking, &bookings[i])) {
            snprintf(response, RESPONSE, "ERROR: the resource is already occupied\n");
            pthread_mutex_unlock(&mutex);
            return;
        }
    }

    bookings[num_bookings] = new_booking;
    num_bookings++;
    save_bookings();
    next_booking_id++;
    snprintf(response, RESPONSE, "OK: request inserted correctly with id %d, waiting for approval\n", new_booking.id);
    pthread_mutex_unlock(&mutex);
}

void cmd_mine(char *response, char *user) {
    int found = 0;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_bookings; i++) {
        if (strcmp(bookings[i].requester, user) == 0) {
            add_booking_row(response, &bookings[i]);
            found++;
        }
    }
    pthread_mutex_unlock(&mutex);
    if (found == 0) {
        snprintf(response, RESPONSE, "No bookings found\n");
    }
}

void cmd_search(char *response, char *user, char *field, char *value) {
    int found = 0;
    if (field == NULL || value == NULL) {
        snprintf(response, RESPONSE, "ERROR: syntax not valid (%s|field|value)\n", CMD_SEARCH);
        return;
    }
    if (strcmp(field, "id") != 0 && strcmp(field, "resource") != 0 &&
        strcmp(field, "date") != 0 && strcmp(field, "status") != 0) {
        snprintf(response, RESPONSE, "ERROR: field not valid (id, resource, date, status)\n");
        return;
    }
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_bookings; i++) {
        Booking *b = &bookings[i];
        if (strcmp(b->requester, user) != 0) continue;
        int match = 0;
        if (strcmp(field, "id") == 0 && b->id == atoi(value))
            match = 1;
        if (strcmp(field, "resource") == 0 && strcmp(b->resource, value) == 0)
            match = 1;
        if (strcmp(field, "date") == 0 && strcmp(b->date, value) == 0)
            match = 1;
        if (strcmp(field, "status") == 0 && strcmp(b->status, value) == 0)
            match = 1;
        if (match) {
            add_booking_row(response, b);
            found++;
        }
    }
    pthread_mutex_unlock(&mutex);
    if (found == 0) {
        snprintf(response, RESPONSE, "No bookings found\n");
    }
}

void cmd_all(char *response) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_bookings; i++) {
        add_booking_row(response, &bookings[i]);
    }
    pthread_mutex_unlock(&mutex);
    if (num_bookings == 0) {
        snprintf(response, RESPONSE, "No bookings found\n");
    }
}

void cmd_status(char *response, char *id_str, char *new_status) {
    if (id_str == NULL || new_status == NULL) {
        snprintf(response, RESPONSE, "ERROR: syntax not valid (%s|id|new_status)\n", CMD_STATUS);
        return;
    }
    if (strcmp(new_status, "pending") != 0 &&
        strcmp(new_status, "approved") != 0 &&
        strcmp(new_status, "refused") != 0) {
        snprintf(response, RESPONSE, "ERROR: status not valid (pending, approved, refused)\n");
        return;
    }
    int id = atoi(id_str);
    int index = -1;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_bookings; i ++) {
        if (bookings[i].id == id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        snprintf(response, RESPONSE, "ERROR: no booking with this id\n");
        pthread_mutex_unlock(&mutex);
        return;
    }

    if (strcmp(new_status, "approved") == 0) {
        for (int i = 0; i < num_bookings; i++) {
            if (i != index &&
                strcmp(bookings[i].status, "approved") == 0 &&
                check_conflicts(&bookings[index], &bookings[i])) {
                snprintf(response, RESPONSE, "ERROR: impossible to approve, there is already an approved booking that collide with this one (%d)\n", bookings[i].id);
                pthread_mutex_unlock(&mutex);
                return;
            }
        }
    }

    strcpy(bookings[index].status, new_status);
    save_bookings();
    snprintf(response, RESPONSE, "OK: booking %d now have status %s\n", id, new_status);
    pthread_mutex_unlock(&mutex);
}

void cmd_conflicts(char *response) {
    int found = 0;
    char row[256];
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_bookings; i++) {
        for (int j = i + 1; j < num_bookings; j++) {
            if (strcmp(bookings[i].status, "refused") == 0 ||
                strcmp(bookings[j].status, "refused") == 0) {
                continue;
            }
            if (check_conflicts(&bookings[i], &bookings[j])) {
                snprintf(row, sizeof(row), "CONFLICT between %d and %d:\n", bookings[i].id, bookings[j].id);
                append_response(response, row);
                append_response(response, "  ");
                add_booking_row(response, &bookings[i]);
                append_response(response, "  ");
                add_booking_row(response, &bookings[j]);
                found++;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    if (found == 0) {
        snprintf(response, RESPONSE, "No conflicts found\n");
    }
}

// CLIENT HANDLING

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER];
    char response[RESPONSE];
    char user[50] = "";
    int admin = 0;

    while(1) {
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            break;
        }
        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        response[0] = '\0';

        char *cmd = strtok(buffer, "|");
        char *a1 = strtok(NULL, "|");
        char *a2 = strtok(NULL, "|");
        char *a3 = strtok(NULL, "|");
        char *a4 = strtok(NULL, "|");

        if (cmd == NULL) {
            snprintf(response, RESPONSE, "ERROR: empty command\n");
        } else if (strcmp(cmd, CMD_REGISTER) == 0) {
            cmd_register(response, a1, a2);
        } else if (strcmp(cmd, CMD_LOGIN) == 0) {
            cmd_login(response, a1, a2, user, &admin);
        } else if (user[0] == '\0') {
            snprintf(response, RESPONSE, "ERROR: access not authorized, login is required\n");
        } else if (strcmp(cmd, CMD_BOOK) == 0) {
            cmd_book(response, user, a1, a2, a3, a4);
        } else if (strcmp(cmd, CMD_MINE) == 0) {
            cmd_mine(response, user);
        } else if (strcmp(cmd, CMD_SEARCH) == 0) {
            cmd_search(response, user, a1, a2);
        } else if (strcmp(cmd, CMD_ALL) == 0 || strcmp(cmd, CMD_STATUS) == 0 || strcmp(cmd, CMD_CONFLICTS) == 0) {
            // commands for admin
            if (!admin) {
                snprintf(response, RESPONSE, "ERROR: this operation is reserved to admin users\n");
            } else if (strcmp(cmd, CMD_ALL) == 0) {
                cmd_all(response);
            } else if (strcmp(cmd, CMD_STATUS) == 0) {
                cmd_status(response, a1, a2);
            } else {
                cmd_conflicts(response);
            }
        } else {
            snprintf(response, RESPONSE, "ERROR: unknown command\n");
        }

        strcat(response, MSG_END);
        send(sock, response, strlen(response), 0);
    }

    printf("Client disconnected (socket %d)\n", sock);
    close(sock);
    return NULL;
}

// MAIN FUNCTION

int main() {
    int server_sock, opt = 1;
    struct sockaddr_in address;

    signal(SIGPIPE, SIG_IGN);

    load_data();

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(server_sock, 10) < 0) {
        perror("listen");
        exit(1);
    }
    printf("Server listening on port %d...\n", PORT);

    while(1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, NULL, NULL);
        if (*client_sock < 0) {
            perror("accept");
            free(client_sock);
            continue;
        }
        printf("New client connected (socket %d)\n", *client_sock);

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0) {
            perror("pthread_create");
            close(*client_sock);
            free(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}

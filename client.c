#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "protocol.h"

// UTILITY FUNCTIONS

void read_input(char *buffer, int size) {
    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

int valid_field(char *s) {
    if (strlen(s) == 0) {
        printf("Empty field\n");
        return 0;
    }
    if (strchr(s, ' ') != NULL || strchr(s, '|') != NULL) {
        printf("The field can't contain spaces or '|'\n");
        return 0;
    }
    return 1;
}

int valid_text(char *s) {
    if (strlen(s) == 0) {
        printf("Empty field\n");
        return 0;
    }
    if (strchr(s, '|') != NULL) {
        printf("The field can't contain '|'\n");
        return 0;
    }
    return 1;
}

int send_command(int sock, char *cmd, char *response) {
    char buffer[BUFFER];
    int n, tot;

    if (send(sock, cmd, strlen(cmd), 0) < 0) {
        printf("Network error: impossible to send the command\n");
        return -1;
    }

    response[0] = '\0';
    while (1) {
        n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Network error: connection with server lost\n");
            return -1;
        }
        buffer[n] = '\0';
        if (strlen(response) + strlen(buffer) >= RESPONSE) {
            printf("Network error: response too large\n");
            return -1;
        }
        strcat(response, buffer);
        tot = strlen(response);
        int end_len = strlen(MSG_END);
        if (tot >= end_len && strcmp(response + tot - end_len, MSG_END) == 0) {
            response[tot - end_len] = '\0';
            return 0;
        }
    }
}

// COMMANDS

int new_booking(int sock, char *response) {
    char resource[50], date[20], start[10], end[10], cmd[BUFFER];

    printf("Resource name (no '|'): ");
    read_input(resource, sizeof(resource));
    if (!valid_text(resource)) return 0;
    printf("Date (dd/mm/yyyy format): ");
    read_input(date, sizeof(date));
    if (!valid_field(date)) return 0;
    printf("Start time (hh:mm format): ");
    read_input(start, sizeof(start));
    if (!valid_field(start)) return 0;
    printf("End time (hh:mm format): ");
    read_input(end, sizeof(end));
    if (!valid_field(end)) return 0;

    sprintf(cmd, "%s|%s|%s|%s|%s", CMD_BOOK, resource, date, start, end);
    return send_command(sock, cmd, response);
}

int search(int sock, char *response) {
    char field[20], value[50], cmd[BUFFER];

    printf("Search field (id, resource, date, status): ");
    read_input(field, sizeof(field));
    if (!valid_field(field)) return 0;
    printf("Value to search (max 50 chars): ");
    read_input(value, sizeof(value));
    if (!valid_text(value)) return 0;

    sprintf(cmd, "%s|%s|%s", CMD_SEARCH, field, value);
    return send_command(sock, cmd, response);
}

// return 0 to continue
int approve_refuse_request(int sock, char *response) {
    char id[20], status[20], cmd[BUFFER];

    printf("Request id: ");
    read_input(id, sizeof(id));
    if (!valid_field(id)) {
        strcpy(response, "ID not valid");
        return 0;
    }
    printf("Approve or refuse? (a/r): ");
    read_input(status, sizeof(status));
    if (strcmp(status, "a") == 0) {
        sprintf(cmd, "%s|%s|approved", CMD_STATUS, id);
    } else if (strcmp(status, "r") == 0) {
        sprintf(cmd, "%s|%s|refused", CMD_STATUS, id);
    } else {
        strcpy(response, "Your choice is not valid");
        return 0;
    }
    return send_command(sock, cmd, response);
}

// return 0 to continue
int edit_status(int sock, char *response) {
    char id[20], status[20], cmd[BUFFER];

    printf("Request id: ");
    read_input(id, sizeof(id));
    if (!valid_field(id)) {
        strcpy(response, "ID not valid");
        return 0;
    }
    printf("Approve or refuse or pending? (a/r/p): ");
    read_input(status, sizeof(status));
    if (strcmp(status, "a") == 0) {
        sprintf(cmd, "%s|%s|approved",CMD_STATUS, id);
    } else if (strcmp(status, "r") == 0) {
        sprintf(cmd, "%s|%s|refused", CMD_STATUS, id);
    } else if (strcmp(status, "p") == 0) {
        sprintf(cmd, "%s|%s|pending", CMD_STATUS, id);
    } else {
        strcpy(response, "Your choice is not valid");
        return 0;
    }

    return send_command(sock, cmd, response);
}

// MENUES

void menu_user(int sock, char *username) {
    char choice[10], response[RESPONSE];

    while (1) {
        printf("\nUSER %s MENU\n", username);
        printf("1. New Booking request\n");
        printf("2. Visualize bookings\n");
        printf("3. Search bookings\n");
        printf("0. Exit\n");
        printf("Choose: ");
        read_input(choice, sizeof(choice));

        if (strcmp(choice, "0") == 0) {
            return;
        } else if (strcmp(choice, "1") == 0) {
            if (new_booking(sock, response) < 0) return;
            printf("%s", response);
        } else if (strcmp(choice, "2") == 0) {
            if (send_command(sock, CMD_MINE, response) < 0) return;
            printf("%s", response);
        } else if (strcmp(choice, "3") == 0) {
            if (search(sock, response) < 0) return;
            printf("%s", response);
        } else {
            printf("Your choice is not valid\n");
        }
    }
}

void menu_admin(int sock) {
    char choice[10], response[RESPONSE];

    while (1) {
        printf("\nADMIN MENU\n");
        printf("1. Visualize all the bookings\n");
        printf("2. Approve/Refuse request\n");
        printf("3. Edit booking status\n");
        printf("4. Verify conflicts\n");
        printf("0. Exit\n");
        printf("Choose: ");
        read_input(choice, sizeof(choice));

        if (strcmp(choice, "0") == 0) {
            return;
        } else if (strcmp(choice, "1") == 0) {
            if (send_command(sock, CMD_ALL, response) < 0) return;
            printf("%s", response);
        } else if (strcmp(choice, "2") == 0) {
            if (approve_refuse_request(sock, response) < 0) return;
            printf("%s", response);
        } else if (strcmp(choice, "3") == 0) {
            if (edit_status(sock, response) < 0) return;
            printf("%s", response);
        } else if (strcmp(choice, "4") == 0) {
            if (send_command(sock, CMD_CONFLICTS, response) < 0) return;
            printf("%s", response);
        } else {
            printf("Your choice is not valid\n");
        }
    }
}

// MAIN

int main (int argc, char *argv[]) {
    int sock;
    struct sockaddr_in address;
    char *ip = "127.0.0.1";
    char choice[50], user[50], pass[50], cmd[BUFFER], response[RESPONSE];

    if (argc > 1) ip = argv[1];

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &address.sin_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect");
        printf("Network error: impossible to connects to %s:%d\n", ip, PORT);
        exit(1);
    }
    printf("Connected to the server %s:%d\n", ip, PORT);

    while (1) {
        printf("\nBOOKING SYSTEM\n");
        printf("1. Login\n");
        printf("2. Register\n");
        printf("0. Exit\n");
        printf("Choose: ");
        read_input(choice, sizeof(choice));

        if (strcmp(choice, "0") == 0) {
            close(sock);
            return 0;
        } else if (strcmp(choice, "1") == 0 || strcmp(choice, "2") == 0) {
            printf("Username: ");
            read_input(user, sizeof(user));
            if (!valid_field(user)) continue;
            printf("Password: ");
            read_input(pass, sizeof(pass));
            if (!valid_field(pass)) continue;

            if (strcmp(choice, "1") == 0)
                sprintf(cmd, "%s|%s|%s", CMD_LOGIN, user, pass);
            else
                sprintf(cmd, "%s|%s|%s", CMD_REGISTER, user, pass);

            if (send_command(sock, cmd, response) < 0) {
                close(sock);
                return 1;
            }
            printf("%s", response);

            if (strncmp(response, RESP_OK_ADMIN, strlen(RESP_OK_ADMIN)) == 0) {
                menu_admin(sock);
                break;
            } else if (strncmp(response, RESP_OK_USER, strlen(RESP_OK_USER)) == 0) {
                menu_user(sock, user);
                break;
            }
        } else {
            printf("Choice not valid\n");
        }
    }

    printf("See you soon!\n");
    close(sock);
    return 0;
}

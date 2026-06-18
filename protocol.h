#ifndef PROTOCOL_H
#define PROTOCOL_H

// Connection
#define PORT 8080
#define BUFFER 1024
#define RESPONSE 32768

// Message framing
#define MSG_END "END\n"

// Login response prefixes
#define RESP_OK_ADMIN "OK ADMIN"
#define RESP_OK_USER  "OK USER"

// CMDs
#define CMD_LOGIN "LOGIN"
#define CMD_REGISTER "REGISTER"
//USER
#define CMD_BOOK "BOOK"
#define CMD_MINE "MINE"
#define CMD_SEARCH "SEARCH"
//ADMIN
#define CMD_ALL "ALL"
#define CMD_STATUS "STATUS"
#define CMD_CONFLICTS "CONFLICTS"

#endif

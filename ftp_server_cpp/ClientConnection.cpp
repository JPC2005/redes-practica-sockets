//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************

#include <cstring>
#include <cstdio>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include "common.h"
#include "ClientConnection.h"

ClientConnection::ClientConnection(int s) {
   control_socket = s;
   // Check the Linux man pages to know what fdopen does.
   control_fd = fdopen(s, "a+");
   if (control_fd == nullptr) {
      std::cout << "Connection closed" << std::endl;
      fclose(control_fd);
      close(control_socket);
      ok = false;
      return;
   }
   ok = true;
   data_socket = -1;
   stop_server = false;
};

ClientConnection::~ClientConnection() {
   fclose(control_fd);
   close(control_socket);
}

int connect_TCP(uint32_t address, uint16_t port) {
   // TODO: Implement your code to define a socket here
  struct sockaddr_in sin;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  
  memcpy(&sin.sin_addr, &address, sizeof(address));

  s = socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0)
    errexit("No se puede crear el socket : %s\n", strerror(errno));

  if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("No se puede conectar con %s: %s\n", address, strerror(errno));

   return s;
}

void ClientConnection::stop() {
   close(data_socket);
   close(control_socket);
   stop_server = true;
}

#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.
void ClientConnection::WaitForRequests() {
   uint32_t ip = -1;
   uint16_t port = -1;
   if (!ok) {
      return;
   }
   fprintf(control_fd, "220 Service ready\n");
   while (!stop_server) {
      fscanf(control_fd, "%s", command);
      if (COMMAND("USER")) {
         fscanf(control_fd, "%s", arg);
         fprintf(control_fd, "331 User name ok, need password\n");
      } else if (COMMAND("PWD")) {
      } else if (COMMAND("PASS")) {
         fscanf(control_fd, "%s", arg);
         if (strcmp(arg, "1234") == 0) {
            fprintf(control_fd, "230 User logged in\n");
         } else {
            fprintf(control_fd, "530 Not logged in.\n");
            stop_server = true;
            break;
         }
      } else if (COMMAND("PORT")) {
        int h1, h2, h3, h4, p1, p2;
        fscanf(control_fd, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
        uint32_t address = h4 << 24 | h3 << 16 | h2 << 8 | h1;
        uint16_t port = p1 << 8 | p2;
        data_socket = connect_TCP(address, port);
        if (data_socket < 0) {
            fprintf(control_fd, "501 PORT Syntax error in parameters or arguments.\n");
        } else {
            fprintf(control_fd, "200 PORT command successful.\n");
        }
      } else if (COMMAND("PASV")) {
         // TODO: To be implemented by students
      } else if (COMMAND("STOR")) {
         // TODO: To be implemented by students
      } else if (COMMAND("RETR")) {
         // TODO: To be implemented by students
      } else if (COMMAND("LIST")) {
          fprintf(control_fd, "125 Data connection already open; transfer starting.\n");
          fflush(control_fd);

          DIR *directory = opendir(get_current_dir_name());
          struct dirent *directory_element;
          char buffer[MAX_BUFF];
          size_t size;

          if (directory != NULL) {
            while ((directory_element = readdir(directory)) != NULL) {
               size = sprintf(buffer, "%s\n", directory_element->d_name);
               send(data_socket, buffer, size, 0);
               }
            } else {
               fprintf(control_fd, "450 Can't open directory.\n");
               close(data_socket);
            }
            fprintf(control_fd, "250 Transfer complete.\n");
            fflush(control_fd);
            closedir(directory);
            close(data_socket);
      } else if (COMMAND("SYST")) {
         fprintf(control_fd, "215 UNIX Type: L8.\n");
      } else if (COMMAND("TYPE")) {
         fscanf(control_fd, "%s", arg);
         fprintf(control_fd, "200 OK\n");
      } else if (COMMAND("QUIT")) {
         fprintf(control_fd, "221 Service closing control connection. Logged out if appropriate.\n");
         close(data_socket);
         stop_server = true;
         break;
      } else {
         fprintf(control_fd, "502 Command not implemented.\n");
         fflush(control_fd);
         printf("Comando : %s %s\n", command, arg);
         printf("Error interno del servidor\n");
      }
   }

   fclose(control_fd);
};

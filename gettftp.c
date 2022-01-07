#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void gettftp(char *host, char *file) {

}

int main(int argc, char *argv[]) {
	//argv0 command entrée, argc nbre d'arguments, argv1 premier argument (host), argv2 deuxième (file)
  if (argc==3) {
	  gettftp(argv[1], argv[2]);
  }
	return 0;
}

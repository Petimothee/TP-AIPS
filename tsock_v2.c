/* librairie standard ... */
#include <stdlib.h>
/* pour getopt */
#include <unistd.h>
/* déclaration des types de base */
#include <sys/types.h>
/* constantes relatives aux domaines, types et protocoles */
#include <sys/socket.h>
/* constantes et structures propres au domaine UNIX */
#include <sys/un.h>
/* constantes et structures propres au domaine INTERNET */
#include <netinet/in.h>
/* structures retournées par les fonctions de gestion de la base de
données du réseau */
#include <netdb.h>
/* pour les entrées/sorties */
#include <stdio.h>
/* pour la gestion des erreurs */
#include <errno.h>
#define DEBUG printf("***DEBUG***\n");

void recv_udp(int);
void envoi_udp(int,struct sockaddr_in, int, int);
void client_tcp(int sock,struct sockaddr_in padr_serv,int lg_m, int nb_messages);
void serveur_tcp(int, int);
int creer_socket(char protocol);
char* construire_message(char*,int, char);
void afficher_message(char *message, int lg);

	

int main (int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;
	int nb_message = -1; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
	int source = -1 ; /* 0=puits, 1=source */
	int port= 9000;
	//char* dest=*argv[argc-2];
	char* dest = argv[argc-1];
	char protocol='t';
	while ((c = getopt(argc, argv, "pn:su")) != -1) {
		switch (c) {
		case 'p':
			if (source == 1) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1);
			}
			source = 0;
			break;

		case 's':
			if (source == 0) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1) ;
			}
			source = 1;
			break;

		case 'n':
			nb_message = atoi(optarg);
			break;

		case 'u':
			protocol ='u';
			printf("Utilisation UDP\n");
			break;

		default:
			printf("Default usage: cmd [-p|-s][-n ##]\n");
			break;
		}
	}


	if (source == -1) {
		printf("usage: cmd [-p|-s][-n ##]\n");
		exit(1) ;
	}

	if (nb_message != -1) {
		if (source == 1)
			printf("nb de tampons à envoyer : %d\n", nb_message);
		else
			printf("nb de tampons à recevoir : %d\n", nb_message);
	} 
	else {
		if (source == 1) {
			nb_message = 10 ;
			printf("nb de tampons à envoyer = 10 par défaut\n");
		} else{
			nb_message = 10;
			printf("nb de tampons à recevoir = infini\n");
		} 
	}

	if (source == 1){ //on envoie le message (source)
		printf("on est dans le source\n");

		int sock= creer_socket(protocol); //on cree le socket local
		struct hostent *hp; //on construit l'adresse du socket auquel on souhaite s'adresser
		struct sockaddr_in adr_distant;
		memset((char *)&adr_distant, 0, sizeof(adr_distant));
		adr_distant.sin_family=AF_INET;
		adr_distant.sin_port=htons(atoi(dest));
		printf("Num port : %d\n", atoi(dest));
		if ((hp = gethostbyname(argv[argc-2])) == NULL) {
			printf("erreur gethostbyname\n");
		    exit(1);
		}
		printf("Nom de la station : %s\n", hp->h_name);
		memcpy((char*)&(adr_distant.sin_addr.s_addr),hp->h_addr,hp->h_length);
		//fin de construction de l'adresse du socket auquel on souhaite s'adresser
		if (protocol == 'u'){
			envoi_udp(sock,adr_distant,30, nb_message);
		}
		else{ //cote client (processus appelant)
			client_tcp(sock, adr_distant, 30, nb_message);
		}

		if (close(sock) == -1){
		    printf("Echec de la destruction du socket\n"); 
		} 
	}


	else {
		printf("on est dans le puits\n");
		if (protocol == 'u'){
			recv_udp(atoi(dest));
       		} 
		else {
            		serveur_tcp(atoi(dest), nb_message);
        	} 
	}

return 0;
} //fin du main

int creer_socket(char protocol){
	int type;
	int protocole;
	int sock;
	
	if (protocol=='u'){
		type=SOCK_DGRAM;
		protocole= IPPROTO_UDP;
	}
	else if (protocol=='t'){
		type=SOCK_STREAM;
		protocole= 0;
	}
	if ((sock = socket(AF_INET, type, protocole)) == -1){
		printf("Echec de la creation du socket\n");
	}
	return sock;
}

char* construire_message(char* message,int lg, char motif) {
	int i;
	for (i=0;i<lg;i++) message[i] = motif;
	return message; 
}
	
void afficher_message(char *message, int lg) {
	int i;
	printf("message construit : "); 
	
	for (i=0;i<lg;i++) {
		printf("%c", message[i]);
	} 
	printf("\n");
}

void envoi_udp(int sock,struct sockaddr_in addresse_dest,int lg_m, int nb_messages ){
	char *msg = malloc(sizeof(lg_m*sizeof(char)));
	
	for (int i=0; i<nb_messages; i++){
	    msg = construire_message(msg, lg_m, (char)(i+97));
	    sendto(sock,msg,lg_m,0,(struct sockaddr*)&addresse_dest,sizeof(addresse_dest));
	    afficher_message(msg,lg_m);
	} 
}

void recv_udp(int num_port){
	int lg_m = 30;
	int sock_local= creer_socket('u');
	struct sockaddr_in addr_local;

	memset((char *)& addr_local, 0, sizeof(addr_local)) ;

	addr_local.sin_family = AF_INET ; /* domaine Internet */
	addr_local.sin_port = htons(num_port) ; /* n° de port */
	addr_local.sin_addr.s_addr = INADDR_ANY;
	int taille=sizeof(addr_local);

	char* mess_recu= malloc(lg_m*sizeof(char));
	struct sockaddr *addr_dest = malloc(sizeof(struct sockaddr));
	int taille2=sizeof(struct sockaddr);

	if (bind(sock_local,(struct sockaddr*)&addr_local,taille) == -1){
	    printf("Echec du bind\n");
	    exit(1);
	}
	
	while (1) {
		if(recvfrom(sock_local,mess_recu,lg_m,0,addr_dest,&taille2)<0){
			perror("Erreur recvfrom");
			exit(1);
		}
		afficher_message(mess_recu,30); 
	}
	
	if (close(sock_local) == -1) {
	    printf("Echec de la destruction du socket\n");
	} 
}

void client_tcp(int sock,struct sockaddr_in padr_serv,int lg_m, int nb_messages){

	int s=-1;
	while (s==-1){
		if ((s = connect(sock, (struct sockaddr*)&padr_serv, sizeof(padr_serv))) == -1) {
			printf("Echec de connexion. Retentative...\n");
			sleep(5);
    		}
	}
	char *msg = malloc(sizeof(lg_m));
	if (s==0) {
    		for (int i=0; i<nb_messages; i++){
			msg = construire_message(msg,lg_m, (char)(i+97));
			int lg_mesg;
			if((lg_mesg = send(sock,msg,lg_m,0)) == -1){
				printf("Echec du send\n");
				exit(1);
			} 
			afficher_message(msg,lg_m);
   		 } 
	} 
}

void serveur_tcp(int num_port, int nb_messages){ //cote qui recoit l'appel de connexion (on a choisit que ce soit le processus lui meme qui traite la requete)
	int sock_local = creer_socket('t'); //creation du socket local
	struct sockaddr_in addr_local; //creation de l'adresse du socket

	memset((char *)&addr_local, 0, sizeof(addr_local)) ;

	addr_local.sin_family = AF_INET ; /* domaine Internet */
	addr_local.sin_port = htons(num_port) ; /* n° de port */
	addr_local.sin_addr.s_addr = INADDR_ANY;
	int taille=sizeof(addr_local);
	//fin de la creation de l'adresse du socket

	if (bind(sock_local,(struct sockaddr*)&addr_local,taille) == -1){
		printf("Echec du bind\n");
		exit(1);
	}

	if (listen(sock_local, 5) == -1){
		printf("Trop de demandes de connexion (>5)\n");
		exit(1);
	} 

	struct sockaddr_in adr_client;
	int taille_client=sizeof(adr_client);
	int sock_connexion;
	
	if ((sock_connexion = accept(sock_local, (struct sockaddr*)&adr_client, &taille_client)) == -1){
		printf("Erreur de l'accept\n");
		exit(1);
	}
	
	int lg_rec;
	int lg_max = 30;
	int i;
	char* mess_recu = malloc(sizeof(lg_max));
	
	for (i=0; i<nb_messages; i++){
		if ((lg_rec = recv(sock_connexion, mess_recu, lg_max, 0))<0){
			printf("Echec du read\n");
			exit(1);
	    	}
		afficher_message(mess_recu, lg_max);
	} 
} 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 8

static pthread_mutex_t verrou_creation = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dispo = PTHREAD_MUTEX_INITIALIZER;;
static int* thread_dispo;
static int nb_dispo;

unsigned int qflag = 0;
unsigned int vflag = 0;
unsigned int lflag = 0;
unsigned int aflag = 0;
unsigned int dflag = 0;
unsigned int nbT = 0;
char* dossierExtraire;

struct header_posix_ustar {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
};

struct dataThread{
	int lecture;
	int size;
	int fd;
	struct header_posix_ustar files_info;
	char* name;
	int num;
};

int octal_to_dec(char *octalChar){
	int dec = 0;
	int octal = atoi(octalChar);
	int nbchiffres=1;
	int d=10;
	while((octal/d)!=0){
		nbchiffres++;
		d*=10;
	}
	int i;
	int digit = 1;
	for(i=0;i<nbchiffres;i++){
		digit = octal%10;
		dec += digit*pow(8,i);
		octal = octal/10;
	}
	return dec;
}

void lister(int fd, struct header_posix_ustar files_info, char* archive){
	int size = 0;
	int lecture = read(fd,&files_info,512);
	size = octal_to_dec(files_info.size);
	if(!qflag)printf("List of directories/file in the archive \"%s\" :\n", archive);
	printf("-%s\n",files_info.name);
	while(lecture != 0){
		size = octal_to_dec(files_info.size);
		if(size!=0){
			lseek(fd,(size/512+1)*512,SEEK_CUR);
		}
		lecture = read(fd,&files_info,512);
		if(strlen(files_info.name)!=0){
			printf("-%s\n",files_info.name);
		}
	}
}

void extraire_dossier(char* archive, char* chemin){

	if(dflag == 2){
				char slash[FILENAME_MAX]="/";
				strcat(chemin,slash);
				strcat(chemin,dossierExtraire);
				if(chdir(chemin)!=0){
					if(mkdir(chemin,00700)==0){
						if(!qflag) printf("directory creation %s\n",dossierExtraire);
						chdir(chemin);
					}
				}
	}else{
		char *d = malloc(FILENAME_MAX*sizeof(char));
		char *archive_dossier = malloc(FILENAME_MAX*sizeof(char));
		strcpy(archive_dossier,archive);
		if((d = strrchr(archive_dossier, '/'))!=NULL){
				strncat(chemin,d,strlen(d)-4);
		}
		else {
				char slash[FILENAME_MAX]="/";
				strcat(slash,archive_dossier);
				strncat(chemin,slash,strlen(slash)-4);
		}
		if(chdir(chemin)!=0){
			if(mkdir(chemin,00700) == 0){
				archive_dossier[strlen(archive_dossier)-4] = '\0';
				if(d != NULL){
					d[strlen(d)-4] = '\0';
					if(!qflag)printf("directory creation %s\n",d+1);
				}
				else {if(!qflag) printf("directory creation %s\n",archive_dossier);}
				chdir(chemin);
				//free(arch_dossier);
			}

		}
	}
	//free(d);
}

void *creation_fichier(void *arg){
	struct dataThread *mine = malloc(sizeof(struct dataThread));
	mine = (struct dataThread *)arg;

	if(!qflag && vflag)printf("Thread n°%d -> file creation %s\n",(mine->num+1),mine->name);
	else if(!qflag && !vflag)printf("file creation %s\n",mine->name);
	int fd_file = creat(mine->name,S_IRWXU);
	int fin = mine->size%512;
	int nb_bloc = mine->size/512;
	int compt_bloc = 0;
	thread_dispo[mine->num] = 0;
	pthread_mutex_unlock(&verrou_creation);

	if(mine->size > 512){
		for(compt_bloc=0;compt_bloc<nb_bloc;compt_bloc++){
			mine->lecture = read(mine->fd,&mine->files_info,512);
			write(fd_file,mine->files_info.name,512);
			fsync(fd_file);
		}
		mine->lecture = read(mine->fd,&mine->files_info,512);
		write(fd_file,mine->files_info.name,fin);
		fsync(fd_file);
	} else {
		mine->lecture = read(mine->fd,&mine->files_info,512);
		write(fd_file,mine->files_info.name,mine->size);
		fsync(fd_file);
	}
	if(!qflag && vflag)printf("Thread n°%d end\n",(mine->num+1));
	close(fd_file);
	thread_dispo[mine->num] = 1;
	nb_dispo++;
	pthread_mutex_unlock(&dispo);
	pthread_exit((void *) 0);
}

void extraire(int fd, struct header_posix_ustar files_info, char* archive, char* arg){
	int lecture = 1;
	int size = 0;
	char *name;
	int creation_dossier = 0;
	char chemin[FILENAME_MAX];
	getcwd(chemin,sizeof(chemin));
	if(dflag > 0){
		creation_dossier = 1;
		extraire_dossier(archive,chemin);
	}
	if(nbT==0){
		nbT=NUM_THREADS;
	}
	char *directory = malloc(FILENAME_MAX*sizeof(char));
	char *dossier = malloc(FILENAME_MAX*sizeof(char));
	char *i;
	int position_courante = 0;

	pthread_t thread[nbT];
	int fds[nbT];

	if(creation_dossier == 1){
		chdir("..");
	}
	int index_thread;
	for(index_thread=0;index_thread<nbT;index_thread++){
		thread_dispo[index_thread] = 1;
		fds[index_thread] = open(archive,O_RDONLY); //ouverture de l'archive
		if(fds[index_thread]==-1){
			if(!qflag)perror("Error fd for the thread");
			exit(0);
		}
	}
	if(creation_dossier == 1){
		chdir(chemin);
	}

	int num_thread = 0;
	while(lecture!=0){
		pthread_mutex_lock(&verrou_creation); // qui se débloque dans la procédure creation_fichier
		if(nb_dispo == 0 || nb_dispo ==1) // s'il ne reste qu'un thread dispo le mutex sera bloqué pour la prochaine itération
			pthread_mutex_lock(&dispo);	// se débloque à la fin de la procédure creation_fichier
		int i_dispo;
		for(i_dispo = 0; i_dispo<nbT; i_dispo++){ // boucle pour récupérer un thread dispo
			if(thread_dispo[i_dispo] == 1){
				num_thread = i_dispo;
				if(!qflag && vflag)printf("Thread n°%d available\n",num_thread+1);
				break;
			}
			else if(!qflag && vflag)printf("Thread n°%d not available\n",i_dispo+1);
		}
		lecture = read(fd,&files_info,512); // on lit l'entete
		size = octal_to_dec(files_info.size);
		name = files_info.name;
		if(strlen(name) != 0){
			// il s'agit d'un dossier
			if(size == 0){
				pthread_mutex_unlock(&verrou_creation);  // comme c'est un dossier on débloque les mutex
				pthread_mutex_unlock(&dispo);
				name[strlen(name)-1] = '\0';
				while((i = strchr(name, '/'))!=NULL){
					int decal = i-name;
					dossier = strndup(name, decal);
					if(chdir(dossier)!=0){
						if(mkdir(dossier,00700) == 0){
							if(!qflag)printf("directory creation %s\n",name);
							chdir(dossier);
						}
					}
					i++;
					strcpy(name,i);
				}
				if(chdir(dossier)!=0){
					if(mkdir(name,00700)== 0){
						if(!qflag)printf("directory creation  %s\n",name);
					}
				}
				chdir(chemin);
			}
			// il s'agit d'un fichier
			else {
				strcpy(directory,name);
				//on créé les dossiers s'il n'existe pas encore
				while((i = strchr(directory, '/'))!=NULL){
					int decal = i-directory;
					dossier = strndup(directory, decal);
					if(chdir(dossier)!=0){
						if(mkdir(dossier,00700) == 0){
							if(!qflag)printf("directory creation %s\n",dossier);
							chdir(dossier);
						}
					}
					i++;
					strcpy(directory,i);
				}
				chdir(chemin);

				nb_dispo--;                    //un thread va être utilisé
				thread_dispo[num_thread] = 0;  // on change la disponibilité du thread quiv a être utilisé
				position_courante = lseek(fd,0,SEEK_CUR);	// debut du fichier courant

				lseek(fds[num_thread],position_courante,SEEK_SET);// on place le curseur au début des données du fichier courant
				lseek(fd,(size/512+1)*512,SEEK_CUR);	// on déplace le descripteur courant vers la prochaine entête
				struct dataThread *data = malloc(sizeof(struct dataThread));
				data->lecture = lecture;
				data->size = size;
				data->fd = fds[num_thread];
				data->files_info = files_info;
				data->name = name;
				data->num = num_thread;

				if(pthread_create (&thread[num_thread], NULL, creation_fichier, (void *)data) !=0)
					if(!qflag)printf("Error thread");
			}
		} else {
			lecture = 0;
		}
	}
	int t,rc,status;
	for(t=0;t<nbT;t++){
		if(thread_dispo[t] == 0 && (rc = pthread_join(thread[t],(void **)&status))){
			if(!qflag)printf("ERROR  join() : %d\n", rc);
			exit(-1);
		}
		if(!qflag && vflag)printf("Join thread %d\n",t+1,status);
	}
	//free(dossier);
	//free(directory);
	for(t=0;t<nbT;t++){
		close(fds[t]);
	}
	if(!qflag)printf("Extraction end !\n");
}

int main(int argc, char *argv[]){
if(argc<2 ){
	printf("ptar need one or further arguments \n You can see help with the option -h\n");
	return 1;
}
	int option;
	char* nbThreads;
	while ((option = getopt (argc, argv, "d:qvlt:h")) != -1){
		switch (option)
		{
		case 'h':
			printf("These options are available:\n -q quiet\n -v verbose\n -l list files in the archive\n -d [directory] extract in a directory \n -t nb_threads define the number of threads\n -h help\n");
		case 'q':
			qflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'd':
			dflag = 1;
			if (optarg[0] != '-' && optarg[strlen(optarg)-4] != '.'){
				dflag = 2;
				dossierExtraire = malloc(sizeof(char)*strlen(optarg));
				dossierExtraire = optarg;
			} else {
				dossierExtraire = "";
			}

			break;
		case 't':
			if (optarg[0] != '-' && optarg[strlen(optarg)-4] != '.'){
				nbT = atoi(optarg);
			} else {
				printf("the option -t need an integer\n");
				return 1;
			}
			break;
		default:
			printf("Wrong argument\n");
			return 1;
		}
	}
	if(nbT >0){
		thread_dispo = malloc(sizeof(int)*nbT);
		thread_dispo[nbT];
		nb_dispo = nbT;
	} else {
		thread_dispo = malloc(sizeof(int)*NUM_THREADS);
		nb_dispo = NUM_THREADS;
	}
	char *archive = argv[argc-1];
	if(strcmp(&archive[strlen(archive)-4],".tar") && strcmp(&archive[strlen(archive)-2],"-h")){
		printf("error, your file is not an tar archive\n");
		exit (1);
	}
	int fd=open(archive,O_RDONLY);
	if(fd==-1){
		if(!qflag) perror("Error Archive ");
		exit(1);
	}
	struct header_posix_ustar files_info;

	//Lister
	if(lflag){
		lister(fd,files_info,archive);
	//Extraire
	} else {
		extraire(fd,files_info,archive,argv[1]);
	}
	close(fd);
	//if(dflag==2)free(dossierExtraire);
	//free(thread_dispo);
	return 0;
}

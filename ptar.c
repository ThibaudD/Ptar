#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 3

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

void *travail(void *null){
	int i;
	double result=0.0;
	for(i=0;i<10000000;i++){
		result=result+(double)random();
		printf("Resultat = %e\n",result);
		pthread_exit((void *) 0);
	}
}

void lister(int fd, struct header_posix_ustar files_info, char* archive){
	int size = 0;
	int lecture = read(fd,&files_info,512);
	printf("Liste des dossiers/fichiers contenus dans l'archive \"%s\" :\n", archive);
	printf("%s\n",files_info.name);
	while(lecture != 0){
		size = octal_to_dec(files_info.size);
		if(size!=0){
			lseek(fd,(size/512+1)*512,SEEK_CUR);
		}
		lecture = read(fd,&files_info,512);
		if(strlen(files_info.name)!=0){
			printf("%s\n",files_info.name);
		}
	}
}

void extraire_dossier(char* archive, char* chemin){

	char *d = malloc(FILENAME_MAX*sizeof(char));
	if((d = strrchr(archive, '/'))!=NULL){
		strncat(chemin,d,strlen(d)-4);
	}
	else {
		char slash[FILENAME_MAX]="/";
		strcat(slash,archive);
		strncat(chemin,slash,strlen(slash)-4);
	}
	printf("chemin : %s\n",chemin);
	if(chdir(chemin)!=0){
		if(mkdir(chemin,00700) == 0){
			char *arch_dossier = malloc(FILENAME_MAX*sizeof(char));
			strcpy(arch_dossier,archive);
			arch_dossier[strlen(arch_dossier)-4] = '\0';
			if(d != NULL){
				d[strlen(d)-4] = '\0';
				printf("creation dossier %s\n",d+1);
			}
			else printf("creation dossier %s\n",arch_dossier);
			chdir(chemin);
			//free(d);
			//free(arch_dossier);
		}
	}
	//free(d);
}

void *creation_fichier(void *arg){
	struct dataThread *mine = malloc(sizeof(struct dataThread));
	mine = (struct dataThread *)arg;
	printf("nom_fichier_dansThread : **%s**\n", mine->name);

	printf("creation fichier %s\n",mine->name);
	int fd_file = creat(mine->name,S_IRWXU);
	int fin = mine->size%512;
	int nb_bloc = mine->size/512;
	int compt_bloc = 0;

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
	close(fd_file);

	pthread_exit (0);
	//*/
}

/*void creation_fichier(int lecture, int size, int fd, struct header_posix_ustar files_info,char* name){

	printf("creation fichier %s\n",name);
	int fd_file = creat(name,S_IRWXU);
	int fin = size%512;
	int nb_bloc = size/512;
	int compt_bloc = 0;

	if(size > 512){
		for(compt_bloc=0;compt_bloc<nb_bloc;compt_bloc++){
			lecture = read(fd,&files_info,512);
			write(fd_file,files_info.name,512);
			fsync(fd_file);
		}
		lecture = read(fd,&files_info,512);
		write(fd_file,files_info.name,fin);
		fsync(fd_file);
	} else {
		lecture = read(fd,&files_info,512);
		write(fd_file,files_info.name,size);
		fsync(fd_file);
	}
	close(fd_file);
}
*/
void extraire(int fd, struct header_posix_ustar files_info, char* archive, char* arg){
	int lecture = 1;
	int size = 0;
	char *name;
	char chemin[FILENAME_MAX];
	getcwd(chemin,sizeof(chemin));
	if(!strcmp(arg,"-xd")){
		extraire_dossier(archive,chemin);
	}


	char *directory = malloc(FILENAME_MAX*sizeof(char));
	char *dossier = malloc(FILENAME_MAX*sizeof(char));
	char *i;
	int position = 0;

	/* Threads
			int rc, t, status;
			pthread_t thread[3];
			pthread_attr_t attr;

			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

			for(t=0; t<NUM_THREADS;t++){
				printf("Creation thread %d\n",t);
				if((rc = pthread_create(&thread[t], &attr, travail, NULL))){
					printf("Erreur de _create() : %d\n", rc);
					exit(-1);
				}
			}
			pthread_attr_destroy(&attr);
			for(t=0; t<NUM_THREADS; t++){
				if(rc = pthread_join(thread[t], (void**)&status)){
					printf("ERREUR de _join() : %d\n", rc);
					exit(-1);
				}
				printf("Rejoint thread %d. (ret=%d)\n", t, status);
			}
			pthread_exit(NULL);
	 */

	while(lecture!=0){
		lecture = read(fd,&files_info,512);
		position +=512;
		size = octal_to_dec(files_info.size);
		name = files_info.name;
		if(strlen(name) != 0){
			// il s'agit d'un dossier
			if(size == 0){
				name[strlen(name)-1] = '\0';
				while((i = strchr(name, '/'))!=NULL){
					int decal = i-name;
					dossier = strndup(name, decal);
					if(chdir(dossier)!=0){
						if(mkdir(dossier,00700) == 0){
							printf("creation dossier %s\n",name);
							chdir(dossier);
						}
					}
					i++;
					strcpy(name,i);
				}
				if(chdir(dossier)!=0){
					if(mkdir(name,00700)== 0){
						printf("creation dossier %s\n",name);
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
							//printf("creation dossier %s\n",dossier);
							chdir(dossier);
						}
					}
					i++;
					strcpy(directory,i);
				}
				chdir(chemin);

				int rc, t, status;
				pthread_t th1;
				int fd_1 = open(archive,O_RDONLY); //ouverture de l'archive
				lseek(fd_1,position,SEEK_SET);// on place le curseur au début des données (marche pas)
				lseek(fd,(size/512+1)*512,SEEK_CUR); // on déplace le descripteur courant vers la prochaine entête
				position +=(size/512+1)*512;

				struct dataThread data;
				data.lecture = lecture;
				data.size = size;
				data.fd = fd_1;
				data.files_info = files_info;
				data.name = name;
				printf("nom_fichier_avantThread : **%s**\n", data.name);
				pthread_create (&th1, NULL, creation_fichier, (void *)&data);
				if(rc = pthread_join(th1, (void**)&status)){ // attend la fin du thread
					printf("ERREUR de _join() : %d\n", rc);
					exit(-1);
				}
				printf("Rejoint thread %d. (ret=%d)\n", t, status);
				//creation_fichier(lecture, size, fd, files_info, name);*/
			}
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("ptar a besoin d'un argument:\n -l pour lister les fichiers \n -x pour extraire \n -xd pour extraire dans un dossier\n");
		return 0;
	}
	char *archive = argv[2];

	int fd=open(archive,O_RDONLY);
	if(fd==-1){
		printf("Archive introuvable\n");
		exit(0);
	}
	struct header_posix_ustar files_info;

	/* Lister */
	if(!strcmp(argv[1],"-l")){
		lister(fd,files_info,archive);
		/* Extraire */
	} else if(!strcmp(argv[1],"-x") || !strcmp(argv[1],"-xd")){
		extraire(fd,files_info,archive,argv[1]);
	}
	close(fd);
	return 0;
}

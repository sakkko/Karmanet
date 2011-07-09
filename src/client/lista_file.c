#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h> 

#include "ioutil.h"


void write_share_file(char *dir_path,int fd){
	char temp[255];
	
	struct dirent *dp;
	
	DIR *dir = opendir(dir_path);
	
	while ((dp=readdir(dir)) != NULL) {
	
	
		if(!strcmp(dp->d_name,".")||!strcmp(dp->d_name,".."))		
			continue;
		
		else if(dp->d_type==4){
		
			strcpy(temp,dir_path);
			sprintf(temp,"%s/%s",dir_path,dp->d_name);
			write_share_file(temp,fd);
		
		}
		else{
			char temp1[255];
			sprintf(temp1,"%s/%s",dir_path,dp->d_name);
			write (fd,temp1,strlen(temp1));
			write (fd,"\n",1);
		}
	}
	
	closedir(dir);

}


void share_file(char *dir_path,char *file_log){
	int fd = open (file_log, O_WRONLY|O_CREAT, 0666);
	write_share_file(dir_path,fd);
	close(fd);
}
//scrivo nel file difference le stringhe non presenti in uno ma nell'altro e viceversa
int control_file (char *dir_path,char * file_log,char * changes){
	int n =0;
	int found = 0;
	char buf [255];
	char temp [255];
	char file_log_temp [255] = "temp.txt";
	share_file(dir_path,file_log_temp); 
	int fd2 = open ("difference.txt",O_WRONLY|O_CREAT, 0666);
	int fd = open (file_log, O_RDWR|O_CREAT, 0666);
	int fd1 = open (file_log_temp, O_RDWR|O_CREAT, 0666);
	//controllo se ho cancellato qualcosa
	
	while ((n = readline(fd, buf, 255)) != 0) {
		found = 0;		
		while ((n = readline(fd1, temp, 255)) != 0) {
							
			if (!strcmp(buf, temp)) { 
				found = 1;
				break;
			}
			
		}
		if (found == 0) {
			write(fd2, buf, strlen(buf));
		}		
		lseek(fd1, 0, SEEK_SET);	
	}
	lseek(fd1, 0, SEEK_SET);
	lseek(fd, 0, SEEK_SET);
	char *files = "file aggiunti:\n";
	write(fd2, files, strlen(files));		
	//controllo se ho aggiunto qualcosa
		while ((n = readline (fd1,temp,255)) != 0) {
			found = 0;		
		while ((n = readline(fd, buf, 255)) != 0) {
							
			if (!strcmp(buf, temp)) { 
				found = 1;
				break;
			}
			
		}
		if (found == 0) {	
			
			write(fd2, temp, strlen(temp));
		}	
		lseek(fd, 0, SEEK_SET);	
	}
	close(fd);
	close(fd1);
	close(fd2);
	return 0;
}

#if 0
int main () {
char * c;
//share_file("/home/matteo/lista_file","./prova.txt");
control_file("/home/matteo/lista_file","./prova.txt",c);

/*
	struct dirent *dp;
	int fd = open ("./prova.txt", O_WRONLY|O_CREAT, 0666);
   //path della cartella in condivisione
 	char *dir_path="/home/matteo/lista_file";
	DIR *dir = opendir(dir_path);
	while ((dp=readdir(dir)) != NULL) {
	printf("%s %d\n",dp->d_name,dp->d_type);
	if()
	
		if(!strcmp(dp->d_name,".")||!strcmp(dp->d_name,".."))
			continue;
		strcat (dp->d_name,"\n");
		write (fd,dp->d_name,strlen(dp->d_name));
		

	}
	closedir(dir);
	close (fd);*/
	return 0;
}
#endif

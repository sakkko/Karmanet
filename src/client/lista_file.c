#include "lista_file.h"

int selfile(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_REG) {
		return 1;
	} else {
		return 0;
	}
}

int seldir(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_DIR) {
		return 1;
	} else {
		return 0;
	}
	
}

int write_share_file(int fd_share, const char *dirname) {
	struct dirent **file_list;
	struct dirent **dir_list;
	int nfile, ndir, i;
	char str[255];
	
	if ((nfile = scandir(dirname, &file_list, selfile, alphasort)) < 0) {
		perror("write_share_file error - scandir failed for file");
		return -1;
	}

	if ((ndir = scandir(dirname, &dir_list, seldir, alphasort)) < 0) {
		perror("write_share_file error - scandir failed for directory");
		return -1;
	}

	if (nfile > 0) {
	/*	snprintf(str, 255, "%s\n", dirname);
		if (write(fd_share, str, strlen(str)) < 0) {
			perror("write_share_file error - write failed");
			return -1;
		}
*/
		for (i = 0; i < nfile; i ++) {
			snprintf(str, 255, "%s\n", file_list[i]->d_name);
			if (write(fd_share, str, strlen(str)) < 0) {
				perror("write_share_file error - write failed");
				return -1;
			}
			free(file_list[i]);
		}
	}

	for (i = 0; i < ndir; i ++) {
		snprintf(str, 255, "%s/%s", dirname, dir_list[i]->d_name);
		if (write_share_file(fd_share, str) < 0) {
			free(dir_list[i]);
			return -1;
		}
		free(dir_list[i]);
	}
	
	return 0;
}

#if 0 
void write_share_filei2(char *dir_path, int fd){
	char temp[255];
	char temp1[255];
	
	struct dirent *dp;
	DIR *dir = opendir(dir_path);
	
	while ((dp = readdir(dir)) != NULL) {
		if(!strcmp(dp->d_name,".")||!strcmp(dp->d_name,"..")) {		
			continue;
		} else if (dp->d_type == DT_DIR) {
			strncpy(temp, dir_path, 255);
			sprintf(temp,"%s/%s", dir_path, dp->d_name);
			write_share_file(temp, fd);
		} else {
			sprintf(temp1,"%s/%s\n", dir_path, dp->d_name);
			write(fd,temp1,strlen(temp1));
		}
	}
	
	closedir(dir);

}
#endif

void share_file(char *dir_path, char *file_log){
	int fd = open (file_log, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	write_share_file(fd, dir_path);
	close(fd);
}

#if 0
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


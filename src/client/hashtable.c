#include "hashtable.h"


void init_hashtable(){

	printf("INIT HASHTABLE\n");

	hash_file = calloc(HASH_PRIME_NUMBER_FILE,4);
	hash_ip = calloc(HASH_PRIME_NUMBER_PEER,4);
	hash_md5 = calloc(HASH_PRIME_NUMBER_FILE,4);


}


void print_results_name(const struct addr_node *results, const char *name) {
	struct in_addr addr;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];

	while (results != NULL) {
		to_hex(md5_hex, results->md5->md5);
		md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
		addr.s_addr = results->ip;
		printf("%s:%u %s %s\n", inet_ntoa(addr), ntohs(results->port), md5_hex, name);
		results = results->next;
	}

}

void print_results_md5(const struct md5_info *results) {
	struct in_addr addr;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];

	if (results == NULL) {
		return;
	}

	to_hex(md5_hex, results->info->addr->md5->md5);
	md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
	while (results != NULL) {
		addr.s_addr = results->info->addr->ip;
		printf("%s:%u %s %s\n", inet_ntoa(addr), ntohs(results->info->addr->port), md5_hex, results->info->file->name);
		results = results->next;
	}

}

struct addr_node *get_by_name(const char *file_name) {
	unsigned int key = filehash(file_name, strlen(file_name));	
	printf("cerco file:%s key:%d\n", file_name, key);
	struct file_node *app = hash_file[key];
	while (app != NULL && app->next != NULL) {
		if  (!strncmp(app->name, file_name, sizeof(file_name))) {
			return app->addr_list;	
		}
		app = app->next;
	}
	if (app != NULL) {
		if (!strncmp(app->name, file_name, sizeof(file_name))) {
			return app->addr_list;	
		}
	}
	printf("non è presente il file\n");
	return NULL;	
}

struct md5_info *get_by_md5(const unsigned char *md5) {
	unsigned int key = filehash((const char *)md5, MD5_DIGEST_LENGTH);	
	struct md5_node *app = hash_md5[key];
	while (app != NULL && app->next != NULL) {
		if  (!memcmp((char *)app->md5, md5, MD5_DIGEST_LENGTH)) {
			return app->md5_info;	
		}
		app = app->next;
	}
	if (app != NULL) {
		if (!memcmp((char *)app->md5, md5, MD5_DIGEST_LENGTH)) {
			return app->md5_info;	
		}
	}
	printf("md5 non presente\n");
	return NULL;	

}

unsigned int filehash(const char* str, unsigned int len){
   const unsigned int fnv_prime = 0x811C9DC5;
   unsigned int hash      = 0;
   unsigned int i         = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash *= fnv_prime;
      hash ^= (*str);
   }
   
   return hash % HASH_PRIME_NUMBER_FILE;
}

int iphash(long ip){
	return ip % HASH_PRIME_NUMBER_PEER;
}

struct file_node *create_new_file_node(const char *str, unsigned long ip, unsigned short port) {
	struct file_node *ret;
	
	ret = (struct file_node *)malloc(sizeof(struct file_node));
	strncpy(ret->name, str, strlen(str));
	ret->name[strlen(str)] = 0;
	ret->addr_list = (struct addr_node *)malloc(sizeof(struct addr_node));
	ret->addr_list->ip = ip;
	ret->addr_list->port = port;
	ret->addr_list->prev = NULL;
	ret->addr_list->next = NULL;
	ret->prev = NULL;
	ret->next = NULL;
	
	return ret;
}

struct addr_node *create_new_addr_node(unsigned long ip, unsigned short port) {
	struct addr_node *ret;
	
	ret = (struct addr_node *)malloc(sizeof(struct addr_node));
	ret->ip = ip;
	ret->port = port;
	ret->prev = NULL;
	ret->next = NULL;

	return ret;
}


struct md5_info *create_new_md5_info(const struct file_info *file_info) {
	struct md5_info *ret = (struct md5_info *)malloc(sizeof(struct md5_info));
	ret->info = (struct file_info *)file_info;
	ret->prev = NULL;
	ret->next = NULL;

	return ret;
}

struct md5_node *create_new_md5_node(const unsigned char *md5, const struct file_info *file_info) {
	struct md5_node *ret = (struct md5_node *)malloc(sizeof(struct md5_node));
	memcpy((char *)ret->md5, md5, MD5_DIGEST_LENGTH);
	ret->md5_info = create_new_md5_info(file_info);
	ret->prev = NULL;
	ret->next = NULL;

	return ret;
}

int insert_file(const char *str, const unsigned char *md5, unsigned long ip, unsigned short port) {
	struct file_node *entry;
	struct file_node *new_file_node;
	struct addr_node *new_addr_node;
	struct md5_node *md5_node;
	struct ip_node *ip_node;
	int key;
	
	key = filehash(str, strlen(str));
	if ((entry = hash_file[key]) == NULL) {
		//CELLA VUOTA
		hash_file[key] = create_new_file_node(str, ip, port);
		ip_node = insert_ip_node(ip, hash_file[key], hash_file[key]->addr_list);
		md5_node = insert_md5_node(md5, ip_node->file_list);
		hash_file[key]->addr_list->md5 = md5_node;
		ip_node->file_list->md5_info = md5_node->md5_info;
	} else {
		//CELLA OCCUPATA - POSSIBILE COLLISIONE
		while (entry->next != NULL) {
			if (!strncmp(entry->name, str, strlen(str))) {
				break;
			}
			entry = entry->next;		
		}
		
		if (strncmp(entry->name, str, strlen(str))) {
			//COLLISIONE
			new_file_node = create_new_file_node(str, ip, port);
			new_file_node->prev = entry;
			new_file_node->next = NULL;
			entry->next = new_file_node;
			ip_node = insert_ip_node(ip, new_file_node, new_file_node->addr_list);
			md5_node = insert_md5_node(md5, ip_node->file_list);
			new_file_node->addr_list->md5 = md5_node;
			ip_node->file_list->md5_info = md5_node->md5_info;

		} else {
			//NO COLLISIONE
			
			
			new_addr_node = entry->addr_list;

			while (new_addr_node != NULL) {
				if (new_addr_node->ip == ip && new_addr_node->port == port) {
					if (!memcmp(new_addr_node->md5->md5, md5, MD5_DIGEST_LENGTH)) {
						return 0;
					}
				}
				new_addr_node = new_addr_node->next;
			}
			
			new_addr_node = create_new_addr_node(ip, port);
			new_addr_node->next = entry->addr_list;
			entry->addr_list->prev = new_addr_node;
			entry->addr_list = new_addr_node;
			ip_node = insert_ip_node(ip, entry, entry->addr_list);
			md5_node = insert_md5_node(md5, ip_node->file_list);
			new_addr_node->md5 = md5_node;
			ip_node->file_list->md5_info = md5_node->md5_info;
		}		
	}
	
	return 0;
}

struct ip_node *create_new_ip_node(const struct file_node *file, const struct addr_node *addr) {
	struct ip_node *ret;

	ret = (struct ip_node *)malloc(sizeof(struct ip_node));
	ret->ip = addr->ip;
	ret->file_list = (struct file_info *)malloc(sizeof(struct file_info));
	ret->file_list->file = (struct file_node *)file;
	ret->file_list->addr = (struct addr_node *)addr;
	ret->file_list->prev = NULL;
	ret->file_list->next = NULL;
	ret->prev = NULL;
	ret->next = NULL;

	return ret;
}

struct md5_node *insert_md5_node(const unsigned char *md5, const struct file_info *file_info) {
	int key;
	struct md5_node *entry;
	struct md5_node *new_md5_node;
	struct md5_info *new_md5_info;

	key = filehash((char *)md5, MD5_DIGEST_LENGTH);
	entry = hash_md5[key];


	if (entry == NULL) {
		//CELLA VUOTA
		entry = create_new_md5_node(md5, file_info);
		hash_md5[key] = entry;
		return entry;
	} else {
		//CELLA OCCUPATA - POSSIBILE COLLISIONE
		while (entry->next != NULL) {
			if (!memcmp(md5, (char *)entry->md5, MD5_DIGEST_LENGTH)) {
				break;
			}
			entry = entry->next;
		}

		if (!memcmp(md5, (char *)entry->md5, MD5_DIGEST_LENGTH)) {
			//NO COLLISIONE
			new_md5_info = create_new_md5_info(file_info);
			new_md5_info->next = entry->md5_info;
			entry->md5_info->prev = new_md5_info;
			entry->md5_info = new_md5_info;
			return entry;
		} else {
			//COLLISIONE
			new_md5_node = create_new_md5_node(md5, file_info);
			new_md5_node->prev = entry;
			entry->next = new_md5_node;
			return new_md5_node;
		}
	}
}

struct ip_node *insert_ip_node(unsigned long ip, const struct file_node *file, const struct addr_node *addr) {
	int key;
	struct ip_node *entry;
	struct ip_node *new_ip_node;
	struct file_info *new_file_info;

	key = iphash(ip);
	entry = hash_ip[key];


	if (entry == NULL) {
		//CELLA VUOTA
		entry = create_new_ip_node(file, addr);
		hash_ip[key] = entry;
		return entry;
	} else {
		//CELLA OCCUPATA - POSSIBILE COLLISIONE
		while (entry->next != NULL) {
			if (entry->ip == addr->ip) {
				break;
			}
			entry = entry->next;
		}

		if (entry->ip == addr->ip) {
			//NO COLLISIONE
			new_file_info = create_new_file_info(file, addr);
			new_file_info->next = entry->file_list;
			entry->file_list->prev = new_file_info;
			entry->file_list = new_file_info;
			return entry;
		} else {
			//COLLISIONE
			new_ip_node = create_new_ip_node(file, addr);
			new_ip_node->prev = entry;
			entry->next = new_ip_node;
			return new_ip_node;
		}
	}
	return NULL;
}

struct file_info *create_new_file_info(const struct file_node *file, const struct addr_node *addr) {
	struct file_info *ret;

	ret = (struct file_info *)malloc(sizeof(struct file_info));
	ret->addr = (struct addr_node *)addr;
	ret->file = (struct file_node *)file;
	ret->prev = NULL;
	ret->next = NULL;
	return ret;
}

int remove_md5_info(struct md5_info *md5_info, struct md5_node *md5_node) {
	if (md5_info->prev == NULL) {
		//PRIMO ELEMENTO
		if (md5_info->next == NULL) {
			//PRIMO E UNICO
			free(md5_info);
			md5_info = NULL;
			remove_md5_node(md5_node);
		} else {
			//PRIMO - NON UNICO
			md5_info->next->prev = NULL;
			md5_node->md5_info = md5_info->next;
			free(md5_info);
			md5_info = NULL;
		}
	} else {
		//NON PRIMO ELEMENTO
		if (md5_info->next == NULL) {
			//ULTIMO ELEMENTO
			md5_info->prev->next = NULL;
			free(md5_info);
			md5_info = NULL;
		} else {
			//NO ULTIMO ELEMENTO
			md5_info->prev->next = md5_info->next;
			md5_info->next->prev = md5_info->prev;
			free(md5_info);
			md5_info = NULL;
		}
	}

	return 0;
}

int remove_addr_node(struct addr_node *addr, struct file_node *file) {
	if (addr->prev == NULL) {
		//PRIMO ELEMENTO
		if (addr->next == NULL) {
			//PRIMO E UNICO
			free(addr);
			addr = NULL;
			remove_file_node(file);
		} else {
			//PRIMO - NON UNICO
			addr->next->prev = NULL;
			file->addr_list = addr->next;
			free(addr);
			addr = NULL;
		}
	} else {
		//NON PRIMO ELEMENTO
		if (addr->next == NULL) {
			//ULTIMO ELEMENTO
			addr->prev->next = NULL;
			free(addr);
			addr = NULL;
		} else {
			//NO ULTIMO ELEMENTO
			addr->prev->next = addr->next;
			addr->next->prev = addr->prev;
			free(addr);
			addr = NULL;
		}
	}

	return 0;
}

int remove_md5_node(struct md5_node *md5_node) {

	int key = filehash((char *)md5_node->md5, MD5_DIGEST_LENGTH);
	if (md5_node->prev == NULL) {
		//PRIMO ELEMENTO
		if (md5_node->next == NULL) {
			//PRIMO E UNICO
			hash_md5[key] = NULL;
			free(md5_node);
			md5_node = NULL;
		} else {
			//PRIMO MA NON UNICO
			md5_node->next->prev = NULL;
			hash_md5[key] = md5_node->next;
			free(md5_node);
			md5_node = NULL;
		}
	} else {
		if (md5_node->next == NULL) {
			//ULTIMO ELEMENTO
			md5_node->prev->next = NULL;
			free(md5_node);
			md5_node = NULL;
		} else {
			//NON ULTIMO ELEMENTO
			md5_node->prev->next = md5_node->next;
			md5_node->next->prev = md5_node->prev;
			free(md5_node);
			md5_node = NULL;
		}
	}
	return 0;
}


int remove_file_node(struct file_node *file) {
	int key = filehash(file->name, strlen(file->name));
	if (file->prev == NULL) {
		//PRIMO ELEMENTO
		if (file->next == NULL) {
			//PRIMO E UNICO
			hash_file[key] = NULL;
			free(file);
			file = NULL;
		} else {
			//PRIMO MA NON UNICO
			file->next->prev = NULL;
			hash_file[key] = file->next;
			free(file);
			file = NULL;
		}
	} else {
		if (file->next == NULL) {
			//ULTIMO ELEMENTO
			file->prev->next = NULL;
			free(file);
			file = NULL;
		} else {
			//NON ULTIMO ELEMENTO
			file->prev->next = file->next;
			file->next->prev = file->prev;
			free(file);
			file = NULL;
		}
	}
	return 0;
}

int remove_ip_node(struct ip_node *ipnode) {
	unsigned long key = iphash(ipnode->ip);
	

	if (ipnode->prev == NULL) {
		//PRIMO ELEMENTO
		if (ipnode->next == NULL) {
			//PRIMO E UNICO
			hash_ip[key] = NULL;
			free(ipnode);
			ipnode = NULL;
		} else {
			//PRIMO MA NON UNICO
	//		printf("REMOVE IP: %d\nKEY: %d\n", ipnode->ip, key);
			ipnode->next->prev = NULL;
			hash_ip[key] = ipnode->next;
			free(ipnode);
			ipnode = NULL;
		}
	} else {
		if (ipnode->next == NULL) {
			//ULTIMO ELEMENTO
			ipnode->prev->next = NULL;
			free(ipnode);
			ipnode = NULL;
		} else {
			//NON ULTIMO
		//	printf("REMOVE IP: %d\nKEY: %d\n", ipnode->ip, key);
			ipnode->prev->next = ipnode->next;
			ipnode->next->prev = ipnode->prev;
			free(ipnode);
			ipnode = NULL;
		}
	}
	return 0;
}

//se rimuovo anche il nodo che contiene la testa della lista ritorna 1
int remove_file_info(struct file_info *fileinfo, struct ip_node *ipnode) {
	if (fileinfo->prev == NULL) {
		//PRIMO ELEMENTO
		if (fileinfo->next == NULL) {
			//PRIMO E UNICO
			free(fileinfo);
			fileinfo = NULL;
			remove_ip_node(ipnode);
			return 1;
		} else {
			//PRIMO MA NON UNICO
			fileinfo->next->prev = NULL;
			ipnode->file_list = fileinfo->next;
			free(fileinfo);
			fileinfo = NULL;
		}
	} else {
		if (fileinfo->next == NULL) {
			//ULTIMO ELEMENTO
			fileinfo->prev->next = NULL;
			free(fileinfo);
			fileinfo = NULL;
		} else {
			//NON ULTIMO
			fileinfo->prev->next = fileinfo->next;
			fileinfo->next->prev = fileinfo->prev;
			free(fileinfo);
			fileinfo = NULL;
		}
	}
	return 0;
}

int remove_all_file(unsigned long ip, unsigned short port){
	struct ip_node *entry;
	struct file_info *file_info_app;
	struct file_info *tmp;
	int key;

	key = iphash(ip);
	entry = hash_ip[key];
	while (entry != NULL) {
		if (entry->ip == ip) {
			break;
		}
		entry = entry->next;
	}
	if (entry == NULL) {
		return 0;
	}

	
	file_info_app = entry->file_list;

	while (file_info_app != NULL) {
		tmp = file_info_app;
		if (file_info_app->addr->port == port) {
			remove_md5_info(file_info_app->md5_info, file_info_app->addr->md5); 
			remove_addr_node(file_info_app->addr, file_info_app->file);
			remove_file_info(file_info_app, entry);
		}
		file_info_app = tmp->next;
	}	

	return 0;
}

int remove_file(unsigned long ip, unsigned short port,  const char *str, const unsigned char *md5) {
	struct ip_node *entry;
	struct file_info *file_info_app;
	int key;

	key = iphash(ip);
	entry = hash_ip[key];
	while (entry != NULL) {
		if (entry->ip == ip) {
			break;
		}
		entry = entry->next;
	}
	if (entry == NULL) {
		return 0;
	}

	file_info_app = entry->file_list;
	
	while (file_info_app != NULL) {
		if (!strncmp(file_info_app->file->name, str, strlen(str)) && file_info_app->addr->port == port 
				&& !memcmp((char *)file_info_app->addr->md5->md5, md5, MD5_DIGEST_LENGTH)) {
			break;
		}
		file_info_app = file_info_app->next;
	}
	
	if (file_info_app == NULL) {
		return 0;
	}

	remove_md5_info(file_info_app->md5_info, file_info_app->addr->md5);
	remove_addr_node(file_info_app->addr, file_info_app->file);
	remove_file_info(file_info_app, entry);	
	
	return 0;
}

void print_file_table() {
	int i, c, f;
	struct file_node *file_app;
	struct addr_node *addr_app;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];

	f = 0;
	for (i = 0; i < HASH_PRIME_NUMBER_FILE; i ++) {
		if ((file_app = hash_file[i]) == NULL) {
			continue;
		}

		f = 1;
		while (file_app != NULL) {
			printf("File name: %s\n", file_app->name);
			addr_app = file_app->addr_list;

			c = 1;
			while (addr_app != NULL) {
				to_hex(md5_hex, addr_app->md5->md5);
				md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
				printf("IP %d: %lu:%u\n", c, addr_app->ip, addr_app->port);
				printf("MD5 %d: %s\n", c, md5_hex);
				addr_app = addr_app->next;
				c ++;
			}
			printf("\n");
			file_app = file_app->next;
		}

	}

	if (f == 0) {
		printf("\nLISTA VUOTA\n");
	}
}

void print_ip_table() {
	int i, c, f;
	struct ip_node *ip_node_app;
	struct file_info *file_info_app;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];

	f = 0;
	for (i = 0; i < HASH_PRIME_NUMBER_PEER; i ++) {
		if ((ip_node_app = hash_ip[i]) == NULL) {
			continue;
		}
		f = 1;
		while (ip_node_app != NULL) {
			printf("IP: %lu\n", ip_node_app->ip);
			file_info_app = ip_node_app->file_list;

			c = 1;
			while (file_info_app != NULL) {
				to_hex(md5_hex, file_info_app->addr->md5->md5);
				md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
				printf("FILE %d: %s\n", c, file_info_app->file->name);
				printf("MD5 %d: %s\n", c, md5_hex);
				file_info_app = file_info_app->next;
				c ++;
			}
			printf("\n");
			ip_node_app = ip_node_app->next;
		}
	}

	if (f == 0) {
		printf("\nLISTA VUOTA\n");
	}
}

void print_md5_table() {
	int i, c, f;
	struct md5_node *md5_node_app;
	struct md5_info *md5_info_app;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];

	f = 0;
	for (i = 0; i < HASH_PRIME_NUMBER_FILE; i ++) {
		if ((md5_node_app = hash_md5[i]) == NULL) {
			continue;
		}
		f = 1;
		while (md5_node_app != NULL) {
			to_hex(md5_hex, md5_node_app->md5);
			md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
			printf("MD5: %s\n", md5_hex);
			md5_info_app = md5_node_app->md5_info;

			c = 1;
			while (md5_info_app != NULL) {
				printf("FILE %d: %s\n", c, md5_info_app->info->file->name);
				printf("ADDR %d: %lu:%u\n", c, md5_info_app->info->addr->ip, md5_info_app->info->addr->port);
				md5_info_app = md5_info_app->next;
				c ++;
			}
			printf("\n");
			md5_node_app = md5_node_app->next;
		}
	}

	if (f == 0) {
		printf("\nLISTA VUOTA\n");
	}
}


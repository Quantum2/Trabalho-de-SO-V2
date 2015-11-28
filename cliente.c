#include "util.h"
#include <signal.h>

//Variaveis
char fifo_name[25];
int fd_servidor,fd_cliente,estado;

void terminaCliente(int sig){
    close(fd_cliente);
    close(fd_servidor);
    unlink(fifo_name);
    exit(0);
}

void RecebeMensagem(int sig)
{
    int fd_cliente,res;
    MSG string;
    
    //Abre fifo para leitura
    
    if((fd_cliente = open(fifo_name,O_RDONLY)) == -1){
        perror("[Erro] Erro ao abrir o fifo!");
        exit(-1);
    }
    
    //Lê dados
    
    if((res = read(fd_cliente,&string,sizeof(string))) == -1){
        perror("[Erro] Erro a ler do fifo!");
        exit(-1);
    }
    close(fd_cliente);
    
    printf("\n[Servidor] Mensagem -> %s \n",string.comando);
    estado = string.estado;
}

int main(){
	MSG msg;
	int read_res,i=1,envia = 0,j=1,pid;
	char string[100],comando[150],comand[100];
	char *cmd[4],*cd[4];
	FILE *f;

        //Recebe mensagem geral mesmo estando 'preso' scanf
        setbuf(stdout,NULL);
        pid = getpid();
        msg.pid_cliente = pid;
	
	estado = AGUARDA_LOGIN;

        //Sinal utilizado para receber mensagem de todos os clientes
        signal(SIGUSR1,RecebeMensagem);
        //Sinal para terminar o cliente
        signal(SIGINT,terminaCliente);
        
	//Cria fifo cliente
	sprintf(fifo_name, CLIENT_FIFO, msg.pid_cliente);
	if (mkfifo(fifo_name, 0777) == -1) {
		perror("\n[Erro] Mkfifo do cliente deu erro");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "\n[Cliente] FIFO do cliente criado com sucesso!");

	// abre o FIFO do servidor p/ escrita
	fd_servidor = open(SERVER_FIFO, O_WRONLY); /* bloqueante */
	if (fd_servidor == -1) {
		fprintf(stderr, "\n[Erro] O servidor não está a correr\n");
		unlink(fifo_name);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "\n[Cliente] FIFO do servidor aberto!\n");

	//Abre fifo do cliente p/escrita
	fd_cliente = open(fifo_name, O_RDWR); /* bloqueante */
	if (fd_cliente == -1) {
		perror("\n[Erro] Erro ao abrir o FIFO do cliente!");
		close(fd_servidor);
		unlink(fifo_name);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "\n[Cliente] FIFO do cliente aberto para Leitura");

	memset(msg.comando, '\0', TAM_MAX);

	while (1) {  
		envia = 0;
      
                
		printf("\n[Cliente] Cmd -> ");
		fgets(comando, TAM_MAX, stdin);
		comando[strlen(comando) - 1] = 0;
                strcpy(comand,comando);
                //Divide comando
		cmd[0] = strtok(comando," ");
		for(i=1;cmd[i-1] != NULL;i++)
			cmd[i] = strtok(NULL," ");
                i--;
                
                msg.pid_cliente = pid;
                
                //Trata dos comandos
                if (cmd[0] != NULL) {
			if (strcasecmp(cmd[0], "fim") == 0){
				printf("\n[Cliente] A sair...\n");
				break;
			}
                        //Caso ainda não tenha sido feito login
			if(estado == AGUARDA_LOGIN)
                        {
				if(strcasecmp(cmd[0], "login") == 0){
					if(i != 3){
						printf("\n[Erro] Formato: Login Username Password\n");
					}
					else{
							strcpy(msg.comando,comand);
							envia = 1;				
							
						
                                                
                                                if(envia == 0)
                                                    printf("[Erro] Jogador inexistente!");
					}			
				}
				else{
					printf("\n[Cliente] Por favor faça login!\n");
				}		
			}
                        //Caso ainda estejam a espera do jogo
			else if(estado == AGUARDA_JOGO)
                        {
                            if(strcasecmp(cmd[0], "logout") == 0 || strcasecmp(cmd[0], "jogar") == 0){
                                strcpy(msg.comando,cmd[0]);
                                envia = 1;
                            }
                            else if ( strcasecmp(cmd[0], "novo") == 0){
                                if(i != 3)
                                    printf("[Erro] Fornato: novo timeout dificuldade");
                                else{
                                    strcpy(msg.comando,comand);
                                    envia = 1;
                                }
                            }
                            else if(strcasecmp(cmd[0], "quem") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else
                                printf("[Cliente] Comandos aceites: 'novo' , 'jogar' , 'logout' ou 'quem'! ");
                        }
                        //Caso já estejam a jogar
                        else if(estado == A_JOGAR)
                        {
                            if(strcasecmp(cmd[0], "desistir") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else if(strcasecmp(cmd[0], "terminar") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else if(strcasecmp(cmd[0], "sair") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else if(strcasecmp(cmd[0], "ver") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else if(strcasecmp(cmd[0], "info") == 0){
                                
                            }
                            else if(strcasecmp(cmd[0], "norte") == 0 || strcasecmp(cmd[0], "sul") == 0 || strcasecmp(cmd[0], "este") == 0 || strcasecmp(cmd[0], "oeste") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else if(strcasecmp(cmd[0], "apanha") == 0){
                                
                            }
                            else if(strcasecmp(cmd[0], "usa") == 0){
                                
                            }
                            else if(strcasecmp(cmd[0], "grita") == 0){
                                if(i!= 2)
                                    printf("[Erro] Formato: grita mensagem!");
                                else{
                                    strcpy(msg.comando,comand);
                                    envia = 1;
                                }                                    
                            }
                            else if(strcasecmp(cmd[0], "diz") == 0){
                                strcpy(msg.comando,comand);
                                envia = 1;
                            }
                            else if(strcasecmp(cmd[0], "ataca") == 0){
                                
                            }
                            else
                                printf("[Erro] Comando não aceite! ");
                        }
                        else
                            printf("Algo está mal!!!");
		}
		if(envia == 1){
			write(fd_servidor, &msg, sizeof(msg));

			read_res = read(fd_cliente, &msg, sizeof(msg));
			if (read_res == sizeof(msg))
				printf("\n[Servidor] Mensagem -> %s \n", msg.comando);
			else
				printf("\n[Erro] Sem resposta ou resposta incompreensivel"
				"[bytes lidos: %d]", read_res);
                        estado = msg.estado;
		}
	}

	close(fd_cliente);
	close(fd_servidor);
	unlink(fifo_name);
	return 0;
}
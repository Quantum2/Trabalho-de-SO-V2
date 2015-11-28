#include "util.h"
#include <signal.h>

int fd_servidor, fd_cliente; /* descritores de ficheiros (pipes) */
Jogador *aux2Jogadores = NULL; //Auxiliar de jogadores 
JogadorLogout *exJogadores = NULL, *auxExJogadores = NULL,*aux2ExJogadores = NULL; //Auxiliar de jogadores logout
Labirinto Lab;
char fifo_name[50];
int running;

void mostra(char *cmd,MSG msg){
    int flg = 0,x,y,i;
    char temp[100];
    if(jogoDecorrer == 1){
        auxJogadores = jogadores;
        while(auxJogadores != NULL){
            if(msg.pid_cliente == auxJogadores->pid_cliente){
                x = auxJogadores->X;
                y = auxJogadores->Y;
                flg = 1;
                break;
            }
            auxJogadores = auxJogadores->Proximo;
        }
        if(flg == 1){

            if (strcmp(cmd,"") == 0){
                sprintf(temp,"\nSala %d\n", Lab.s[x][y].num);
                strcpy(msg.comando,temp);
                strcat(msg.comando,"Portas:");

                if (Lab.s[x][y].portaN == 1)
                            strcat(msg.comando," Norte ");
                if (Lab.s[x][y].portaS == 1)
                            strcat(msg.comando," Sul ");
                if (Lab.s[x][y].portaO == 1)
                            strcat(msg.comando," Oeste ");
                if (Lab.s[x][y].portaE == 1)
                            strcat(msg.comando," Este ");
                strcat(msg.comando,"\nTem: \nAnimais: ");
                for (i = 0; i < 2; i++){
                    if (strcasecmp(Lab.s[x][y].mons[i].nome, "") != 0){
                        sprintf(temp," %s ", Lab.s[x][y].mons[i].nome);
                        strcat(msg.comando,temp);
                    }
                }
                strcat(msg.comando,"\nObjectos: ");
                for (i = 0; i < 5; i++)
                    if (strcasecmp(Lab.s[x][y].obj[i].nome, "") != 0){
                        sprintf(temp," %s ", Lab.s[x][y].obj[i].nome);
                        strcat(msg.comando,temp);
                    }
            }
        }
    }
    else{
        strcpy(msg.comando,"Só pode executar este comando quando o jogo estiver a decorrer!");
    }
    //Abre cliente e envia-lhe mensagem
    sprintf(fifo_name, CLIENT_FIFO, msg.pid_cliente);
    escreveFifo(fifo_name,msg);

}

//Gera numero aletorio
void iniciaGerador(){
    srand(time(NULL));
}

int geraNumAletorio(int min,int max){
    return (rand() % (max-min+1) + min);
}

//Função chamada ao fim do alarme

void iniciaJogo(int sig){

    //Caso não haja jogadores suficientes
    if(totalJogadores < 2)
    {
        jogoDecorrer = 0;
        jogoCriado = 0;
        totalJogadores = 0;
        manda_mensagem_todos("Jogo não iniciado por não haverem 2 jogadores!",0,2);
        
        auxJogadores = jogadores;
        while(auxJogadores != NULL)
        {
            if(auxJogadores->aJogar == 1 || auxJogadores->admin == 1)
            {
                auxJogadores->aJogar = 0;
                auxJogadores->admin = 0;
                actualizaEstado(auxJogadores->pid_cliente,2);
            }
            auxJogadores = auxJogadores->Proximo;
        }
        
    }
    else
    {
        //Define-se a sala inicial para todos os jogadores
        //Atribui-se os items iniciais
        
        //Aqui vem a nossa logica de jogo
    }
}

//Escreve a mensagem para o Fifo passado por parametro

void escreveFifo(char *fifo,MSG string){     
    
    int fd_cliente;
    
    //Abre fifo do cliente p/ write 
    if((fd_cliente = open(fifo,O_WRONLY)) == -1){
        perror("Erro ao abrir Fifo!");
        exit(0);
    }
    
    //Escreve no fifo
    
    if(write(fd_cliente,&string,sizeof(string)) == -1){
        perror("Erro a escrever no fifo!");
        exit(0);
    }
    //Fecha cliente (close)
    close(fd_cliente);
    fprintf(stderr, "\n[Servidor] Fifo do cliente fechado \n");
}

//Actualiza estado
void actualizaEstado(int pid,int estado){
    char fifo_cli[100];
    MSG string;

    string.estado = estado;
    strcpy(string.comando,"Estado actualizado!");
    
    kill(pid,SIGUSR1);
    sprintf(fifo_cli,CLIENT_FIFO,pid);
    escreveFifo(fifo_cli,string);
}

//Envia mensagem a todos os jogadores
void manda_mensagem_todos(char *mensagem,int pidNaoRecebe,int estado){
    
    char fifo_cli[100];
    //Mensagem
    MSG string;
    
    auxJogadores = jogadores;
    if(pidNaoRecebe != 0){
        while(auxJogadores != NULL){
            if(auxJogadores->pid_cliente == pidNaoRecebe){
                strcpy(string.comando,auxJogadores->nome);
                break;
            }
            auxJogadores = auxJogadores->Proximo; 
        }    
        strcat(string.comando," : ");
        strcat(string.comando,mensagem);
    }
    else
        strcpy(string.comando,mensagem);
    
    string.estado = estado;
    
    //Percorre os jogadores
    
    auxJogadores = jogadores;
    
    while(auxJogadores != NULL)
    {
        //Não envia a mensagem ao pid referido na "pidNaoRecebe"
        if(pidNaoRecebe != 0 && auxJogadores->pid_cliente == pidNaoRecebe){
            auxJogadores = auxJogadores->Proximo;
            continue;
        }
        
        //Envia mensagem aos utilizadores que estao a jogar
        if(auxJogadores->aJogar == 1)
        {
            kill(auxJogadores->pid_cliente,SIGUSR1);
            sprintf(fifo_cli,CLIENT_FIFO,auxJogadores->pid_cliente);
            escreveFifo(fifo_cli,string);
        }
        auxJogadores = auxJogadores->Proximo;
    }    
}

//Processa comandos
void comandos(MSG *msg){
	int i,j,jogadorExiste,flg = 0,x=0,y=0,pid;
	char *cmd[4],temp[TAM_MAX],fifo_name[TAM_MAX];
        FILE *f;
        
        //dividir frases
	cmd[0] = strtok(msg->comando, " "); 

	for (i = 1; cmd[i - 1] != NULL; i++)
		cmd[i] = strtok(NULL, " ");
        i--;
        
        
        //Processa comandos
        
        //LOGIN
	if(strcasecmp(cmd[0],"login") == 0)
        {
            jogadorExiste = 0;

            //Valida ese já existe um jogador com esse nome
            if( jogadores != NULL)
            {
                    auxJogadores = jogadores;
                    while(auxJogadores != NULL){
                            if(strcasecmp(auxJogadores->nome,cmd[1]) == 0 && strcasecmp(auxJogadores->pass,cmd[2]) == 0){
                                    jogadorExiste = 1;
                                    break;
                            }
                            auxJogadores = auxJogadores->Proximo;
                    }
            }

            //Caso o Jogador nao exista
            if(jogadorExiste == 0)
            {
                    //Alloca espaço em memoria para o jogador			
                    aux2Jogadores = (Jogador *) malloc(sizeof(Jogador));

                    //Preenche dados do jogador
                    aux2Jogadores->pid_cliente = msg->pid_cliente;
                    strcpy(aux2Jogadores->nome,cmd[1]);
                    strcpy(aux2Jogadores->pass,cmd[2]);
                    aux2Jogadores->aJogar = 0;
                    aux2Jogadores->vitorias = 0;
                    aux2Jogadores->Proximo = NULL;

                    //Caso jogador ja tenha estado logago actualiza o seu num de vitorias		
                    auxExJogadores = exJogadores;

                    while(auxExJogadores != NULL)
                    {
                            if(strcasecmp(auxExJogadores->nome,cmd[1]) == 0){
                                    aux2Jogadores->vitorias = auxExJogadores->vitorias;
                                    break;					
                            }
                            auxExJogadores = auxExJogadores->Proximo;			
                    }
                    //Liberta espaço em memória o ex jogador
                    free(auxExJogadores);

                    //Adiciona novo jogador ao fim dos "Jogadores"
                    if(jogadores == NULL)
                            jogadores = aux2Jogadores;
                    else
                    {
                            auxJogadores = jogadores;
                            while(auxJogadores->Proximo != NULL)
                            {
                                    auxJogadores = auxJogadores->Proximo;				
                            }
                            auxJogadores->Proximo = aux2Jogadores;
                    }
                    strcpy(msg->comando,"Login efectuado com sucesso!");
                    msg->estado = 2;
                    //Abre cliente e envia-lhe mensagem
                    sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                    escreveFifo(fifo_name,*msg);
            }
            else{
                    strcpy(msg->comando,"Jogador já ligado!");
                    msg->estado = 1;
                    //Abre cliente e envia-lhe mensagem
                    sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                    escreveFifo(fifo_name,*msg);
            }

    }
    //LOGOUT
    else if(strcasecmp(cmd[0],"logout") == 0)
    {

            auxJogadores = jogadores;
            while( auxJogadores != NULL && auxJogadores->pid_cliente != msg->pid_cliente)
                    auxJogadores = auxJogadores->Proximo;


            strcpy(msg->comando,"Logout efectuado com sucesso!");

            //Abre cliente e envia-lhe mensagem
            sprintf(fifo_name, CLIENT_FIFO, auxJogadores->pid_cliente);
            escreveFifo(fifo_name,*msg);

            //termina cliente
            kill(auxJogadores->pid_cliente,SIGINT);


            // Guarda login do jogador, para ser recuperado mais tarde
            aux2ExJogadores = (JogadorLogout *) malloc (sizeof(JogadorLogout));

            // Prenche campos
            strcpy(aux2Jogadores->nome,auxJogadores->nome);	
            strcpy(aux2Jogadores->pass,auxJogadores->pass);
            aux2Jogadores->vitorias = auxJogadores->vitorias;
            aux2ExJogadores->Proximo = NULL;

            //Insere ex jogador na lista exjogadores		
            if(exJogadores == NULL)
                    exJogadores = aux2ExJogadores;
            else{
                    auxExJogadores = exJogadores;
                    while(exJogadores->Proximo != NULL)
                            auxExJogadores = auxExJogadores->Proximo;	
                    auxExJogadores->Proximo = aux2ExJogadores;	
            }

            //remove jogador da lista de jogadores
            auxJogadores = jogadores;
            aux2Jogadores = NULL;
            while(auxJogadores != NULL)
            {
                    if(auxJogadores->pid_cliente == msg->pid_cliente)
                    {
                            if(aux2Jogadores == NULL)
                                    jogadores = auxJogadores->Proximo;
                            else
                                    aux2Jogadores->Proximo = auxJogadores->Proximo;
                            break;			
                    }
                    aux2Jogadores = auxJogadores;
                    auxJogadores = auxJogadores->Proximo;		
            }
    }
    //NOVO
    else if (strcasecmp(cmd[0],"novo") == 0){

        //Caso já tenha sido criado um jogo
        if(jogoCriado == 1)
        {
            strcpy(msg->comando,"Já existe um jogo criado!");
            //Abre cliente e envia-lhe mensagem
            sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }
        else
        {
            if(!atoi(cmd[1]))
            {
                strcpy(msg->comando,"O timeout tem de ser um valor inteiro!");
                //Abre cliente e envia-lhe mensagem
                sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                escreveFifo(fifo_name,*msg);
            }
            else
            {

                if(!atoi(cmd[2]))
                    strcpy(Lab.criado,cmd[2]);
                else
                    strcpy(Lab.criado,"");
                
                jogoCriado = 1;
                totalJogadores = 1;
                alarm(atoi(cmd[1]));

                //Envia mensagem ao cliente
                sprintf(temp,"Caso existam pelo menos 2 jogadores, o jogo começara em %s segundos.",cmd[1]);
                strcpy(msg->comando,temp);
                //Avança de estado para o estado de jogo criado
                msg->estado = 3;

                //Abre cliente e envia-lhe mensagem
                sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                escreveFifo(fifo_name,*msg);

                //Define o jogador como o 'Primeiro jogador

                auxJogadores = jogadores;
                while(auxJogadores != NULL)
                {
                    if(auxJogadores->pid_cliente == msg->pid_cliente)
                    {
                        auxJogadores->admin = 1;
                        auxJogadores->aJogar = 1;
                        break;
                    }
                    auxJogadores = auxJogadores->Proximo;
                }    

            }

        }

    }
    //JOGAR
    else if (strcasecmp(cmd[0],"jogar") == 0)
    {
        //Caso o jogo ainda não tenha sido criado
        if(jogoCriado != 1)
        {
            strcpy(msg->comando,"Jogo nao criado!");
            //Abre cliente e envia-lhe mensagem
            sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }
        //Caso o jogo tenha sido criado mas ainda não esteja a decorrer
        else if(jogoCriado == 1 && jogoDecorrer != 1)
        {
            //Caso já tenha sido atengido o numero máximo de jogadores
            if(totalJogadores == 10)
            {
                strcpy(msg->comando,"Limite máximo de jogadores atingido!");
                //Abre cliente e envia-lhe mensagem
                sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                escreveFifo(fifo_name,*msg);
            }
            else
            {
                auxJogadores = jogadores;
                //Adiciona jogadores ao jogo
                while(auxJogadores != NULL)
                {
                    if(auxJogadores->pid_cliente == msg->pid_cliente)
                    {
                        auxJogadores->aJogar = 1;
                        sprintf(temp,"Jogador '%s' entrou no jogo!",auxJogadores->nome);
                        break;
                    }
                    auxJogadores = auxJogadores->Proximo;
                }
                totalJogadores++;
                manda_mensagem_todos(temp,msg->pid_cliente,3);

                strcpy(msg->comando,"Entrou no jogo!");
                msg->estado = 3;
                //Abre cliente e envia-lhe mensagem
                sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                escreveFifo(fifo_name,*msg);
            }
        }                
        else if(jogoCriado == 1 && jogoDecorrer == 1)
        {
            strcpy(msg->comando,"Espere até outro jogo começar!");
            sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }
    }
    else if (strcmp(cmd[0], "ver") == 0){
        if(jogoDecorrer == 1){
            if(i == 1){
                mostra("",*msg);
            }
            //FALTA DJSAKDJSKAJDAKSDJAKSJDKASJDSKJSAKAD
        }
        else
        {
            strcpy(msg->comando,"Só quando o jogo estiver a decorrer é que pode utilizar este comando!");
            printf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }  
    }		
    else if (strcmp(cmd[0],"quem") == 0){
        auxJogadores = jogadores;
        strcpy(msg->comando,"Jogadores Ligados: ");

        while(auxJogadores != NULL){
            strcat(msg->comando,auxJogadores->nome);
            strcat(msg->comando," ");
            auxJogadores = auxJogadores->Proximo;
        }
        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }
    else if (strcmp(cmd[0],"grita") == 0){
        manda_mensagem_todos(cmd[1],msg->pid_cliente,3);
        strcpy(msg->comando,"Mensagem enviada!");
        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }
    else if (strcmp(cmd[0],"terminar") == 0){
        flg = 0;
        strcpy(msg->comando,"Não tem permissão!");
        auxJogadores = jogadores;
        while(auxJogadores != NULL){
            if(auxJogadores->pid_cliente == msg->pid_cliente && auxJogadores->admin == 1){
                flg = 1;
                strcpy(msg->comando,"Jogo terminado!");
                msg->estado = 2;
                break;
            }
            auxJogadores = auxJogadores->Proximo;
        }
        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);

        if(flg == 1){
            jogoCriado = 0;
            jogoDecorrer = 0;
            totalJogadores = 0;
            manda_mensagem_todos("Jogo terminado por ordem do 'primeiro jogador'!",msg->pid_cliente,2);

            auxJogadores = jogadores;
            while(auxJogadores != NULL)
            {
                if(auxJogadores->aJogar == 1 || auxJogadores->admin == 1)
                {
                    auxJogadores->aJogar = 0;
                    auxJogadores->admin = 0;
                    actualizaEstado(auxJogadores->pid_cliente,2);
                }
                auxJogadores = auxJogadores->Proximo;
            }   
        }
    }            
    else if (strcmp(cmd[0],"diz") == 0){
        flg = 0;
        if(jogoDecorrer == 1){
            auxJogadores = jogadores;
            while(auxJogadores != NULL)
            {
                if(auxJogadores->pid_cliente == msg->pid_cliente)
                {
                    if(auxJogadores->aJogar == 1)
                    {
                        x = auxJogadores->X;
                        y = auxJogadores->Y;
                        strcpy(temp,auxJogadores->nome);
                        flg = 1;
                        break;
                    }
                    else
                    {
                        strcpy(msg->comando,"Não está a jogar!");
                    }

                }
                auxJogadores = auxJogadores->Proximo;
            }
            if(flg == 1){
                flg = 0;

                auxJogadores = jogadores;
                while(auxJogadores!= NULL)
                {
                    if(auxJogadores->pid_cliente != msg->pid_cliente && auxJogadores->X == x && auxJogadores->Y == y){
                        strcat(temp,": ");
                        strcat(temp,cmd[1]);
                        strcpy(msg->comando,temp);
                        //Envia mensagem
                        kill(auxJogadores->pid_cliente,SIGUSR1);
                        sprintf(fifo_name,CLIENT_FIFO,auxJogadores->pid_cliente);
                        escreveFifo(fifo_name,*msg);   
                        flg = 1;
                    }
                    auxJogadores = auxJogadores->Proximo;
                }
                if(flg == 1)
                    strcpy(msg->comando,"Mensagem enviada!");
                else
                    strcpy(msg->comando,"Não havia ninguém na sala! Mensagem não enviada!");
                sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
                escreveFifo(fifo_name,*msg);
            }
        }
        else
        {
            strcpy(msg->comando,"Só quando o jogo estiver a decorrer é que pode utilizar este comando!");
            sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }     

    }
    else if(strcasecmp(cmd[0],"sair") == 0){
        if(jogoDecorrer == 1){
            flg = 0;
            auxJogadores = jogadores;
            while(auxJogadores != NULL){
                if(auxJogadores->pid_cliente == msg->pid_cliente){
                    if(Lab.iX == auxJogadores->X && Lab.iY == auxJogadores->Y)
                        flg = 1;
                    break;
                }
                auxJogadores = auxJogadores->Proximo;
            }
            if(flg == 1)
            {
                jogoCriado = 0;
                jogoDecorrer = 0;
                totalJogadores = 0;
                sprintf(temp,"O jogador %s saiu do jogo! Jogo terminou!",auxJogadores->nome);
                manda_mensagem_todos(temp,msg->pid_cliente,AGUARDA_JOGO);

                auxJogadores = jogadores;
                while(auxJogadores != NULL)
                {
                    if(auxJogadores->aJogar == 1 || auxJogadores->admin == 1)
                    {
                        auxJogadores->aJogar = 0;
                        auxJogadores->admin = 0;
                        actualizaEstado(auxJogadores->pid_cliente,AGUARDA_JOGO);
                    }
                    auxJogadores = auxJogadores->Proximo;
                } 
                strcpy(msg->comando,"Saiu do jogo!");
                msg->estado = AGUARDA_JOGO;
            }
            else
            {
                strcpy(msg->comando,"Não está na sala inicio do labirinto!");
            }
            //Abre cliente e envia-lhe mensagem
            sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }
        else
        {
            strcpy(msg->comando,"Só quando o jogo estiver a decorrer é que pode utilizar este comando!");
            printf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        } 
    }
    else if(strcasecmp(cmd[0],"desistir") == 0){
        flg = 0;
        auxJogadores = jogadores;
        while(auxJogadores != NULL){
            if(msg->pid_cliente == auxJogadores->pid_cliente)
            {
                if(auxJogadores->aJogar == 1)
                    flg = 1;
                break;
            }
            auxJogadores = auxJogadores->Proximo;
        }
        if(flg == 1)
        {
            totalJogadores--;
            if(totalJogadores == 0){
                jogoCriado = 0;
                jogoDecorrer = 0;
            }
             
            if(auxJogadores->admin == 1){
                msg->estado = 2;
                strcpy(msg->comando,"Desistiu do jogo!");
                //Abre cliente e envia-lhe mensagem
                sprintf(fifo_name, CLIENT_FIFO,msg->pid_cliente);
                escreveFifo(fifo_name,*msg);
                
                auxJogadores->aJogar = 0;
                auxJogadores->admin = 0; 

                sprintf(temp,"O jogador '%s' desistiu do jogo!",auxJogadores->nome);
                manda_mensagem_todos(temp,msg->pid_cliente,A_JOGAR); 

                auxJogadores = jogadores;
                while(auxJogadores != NULL)
                {
                    if(auxJogadores->aJogar == 1){
                        auxJogadores->admin = 1;
                        strcpy(msg->comando,"É o novo administrador!");
                        //Envia mensagem
                        kill(auxJogadores->pid_cliente,SIGUSR1);
                        sprintf(fifo_name,CLIENT_FIFO,auxJogadores->pid_cliente);
                        escreveFifo(fifo_name,*msg);

                        break;
                    }
                    auxJogadores = auxJogadores->Proximo;
                }
            }
            else
            {
                msg->estado = 2;
                strcpy(msg->comando,"Desistiu do jogo!");
                //Abre cliente e envia-lhe mensagem
                sprintf(fifo_name, CLIENT_FIFO,msg->pid_cliente);
                escreveFifo(fifo_name,*msg);

                auxJogadores->aJogar = 0;
                auxJogadores->admin = 0;  

                sprintf(temp,"O jogador '%s' desistiu do jogo!",auxJogadores->nome);
                manda_mensagem_todos(temp,msg->pid_cliente,A_JOGAR);
            }
        }
        else{
            strcpy(msg->comando,"Só quando estiver num jogo é que pode utilizar este comando!");
            //Abre cliente e envia-lhe mensagem
            sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
            escreveFifo(fifo_name,*msg);
        }
    }
    else if(strcasecmp(cmd[0],"norte") == 0){
        if(jogoDecorrer == 1){
            strcpy(msg->comando,"Erro!");
            auxJogadores = jogadores;
            while(auxJogadores != NULL){
                if(auxJogadores->pid_cliente == msg->pid_cliente){
                    x = auxJogadores->X;
                    y = auxJogadores->Y;

                    if(Lab.s[x][y].portaN == 1){
                        x--;
                        auxJogadores->X = x;
                        strcpy(msg->comando,"Movimento bem sucedido!");
                        break;  
                    }
                    else
                    {
                        strcpy(msg->comando,"Impossivel mover para norte!");
                        break;  
                    }
                }
                auxJogadores = auxJogadores->Proximo;
            }   
        }
        else
            strcpy(msg->comando,"Só pode executar este comando quando o jogo estiver a decorrer!");
        
        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }
    else if(strcasecmp(cmd[0],"sul") == 0){
        if(jogoDecorrer == 1){
            strcpy(msg->comando,"Erro!");
            auxJogadores = jogadores;
            while(auxJogadores != NULL){
                if(auxJogadores->pid_cliente == msg->pid_cliente){
                    x = auxJogadores->X;
                    y = auxJogadores->Y;

                    if(Lab.s[x][y].portaS == 1){
                        x++;
                        auxJogadores->X = x;
                        strcpy(msg->comando,"Movimento bem sucedido!");
                        break;  
                    }
                    else
                    {
                        strcpy(msg->comando,"Impossivel mover para sul!");
                        break;  
                    }
                }
                auxJogadores = auxJogadores->Proximo;
            }
        }
        else
            strcpy(msg->comando,"Só pode executar este comando quando o jogo estiver a decorrer!");

        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }
    else if(strcasecmp(cmd[0],"este") == 0){
        if(jogoDecorrer == 1){
            strcpy(msg->comando,"Erro!");
            auxJogadores = jogadores;
            while(auxJogadores != NULL){
                if(auxJogadores->pid_cliente == msg->pid_cliente){
                    x = auxJogadores->X;
                    y = auxJogadores->Y;

                    if(Lab.s[x][y].portaE == 1){
                        y++;
                        auxJogadores->Y = y;
                        strcpy(msg->comando,"Movimento bem sucedido!");
                        break;  
                    }
                    else{
                        strcpy(msg->comando,"Impossivel mover para este!");
                        break;  
                    }

                }
                auxJogadores = auxJogadores->Proximo;
            }
        }
        else
            strcpy(msg->comando,"Só pode executar este comando quando o jogo estiver a decorrer!");

        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }
    else if(strcasecmp(cmd[0],"oeste") == 0){
        if (jogoDecorrer == 1){
            strcpy(msg->comando,"Erro!");
            auxJogadores = jogadores;
            while(auxJogadores != NULL){
                if(auxJogadores->pid_cliente == msg->pid_cliente){
                    x = auxJogadores->X;
                    y = auxJogadores->Y;

                    if(Lab.s[x][y].portaO == 1){
                        y--;
                        auxJogadores->Y = y;
                        strcpy(msg->comando,"Movimento bem sucedido!");
                        break;  
                    }
                    else
                    {
                        strcpy(msg->comando,"Impossivel mover para oeste!");
                        break;  
                    }
                }
                auxJogadores = auxJogadores->Proximo;
            }
        }
        else
            strcpy(msg->comando,"Só pode executar este comando quando o jogo estiver a decorrer!");

        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }
    else
    {
        strcpy(msg->comando,"O comando não existe!");
        //Abre cliente e envia-lhe mensagem
        sprintf(fifo_name, CLIENT_FIFO, msg->pid_cliente);
        escreveFifo(fifo_name,*msg);
    }

}

void *thread1(){
	char string[100],comando[150],comand[100];
	char *cmd[4],*cd[4];
	int i;
	
	fprintf(stderr, "\n[Servidor] Thread criada ! \n");
	
	sleep(2);
	
	while(running == 1){
	fprintf(stderr, "\n[Servidor] -> ");
	
	fgets(comando, TAM_MAX, stdin);
	comando[strlen(comando) - 1] = 0;
    strcpy(comand,comando);
    //Divide comando
	cmd[0] = strtok(comando," ");
	for(i=1;cmd[i-1] != NULL;i++)
		cmd[i] = strtok(NULL," ");
			
	//Trata dos comandos
        if (cmd[0] != NULL) {
			if (strcasecmp(cmd[0], "shutdown") == 0){
				printf("\n[Servidor] A sair...\n");
				close(fd_servidor);
				unlink(SERVER_FIFO);
				exit(0);
			}
		}
		if (cmd[0] != NULL) {
			if (strcasecmp(cmd[0], "users") == 0){
				char temp[150];
			
				auxJogadores = jogadores;
        		strcpy(temp,"Jogadores Ligados: ");

        		while(auxJogadores != NULL){
            		strcat(temp,auxJogadores->nome);
            		strcat(temp," ");
            		auxJogadores = auxJogadores->Proximo;
        		}
        		
        		fprintf(stderr, "%s", temp);
			}
		}
	}
	pthread_exit(NULL);
}

int main(){
	MSG msg;
	int res;
	char comando[150];
	
	pthread_t thread;
	pthread_attr_t attr;
	
	running = 1;
	int t1 = pthread_create(&thread, NULL, thread1, NULL);
	
        iniciaGerador();
       
        if(signal(SIGALRM,iniciaJogo) == SIG_ERR)
        {
            perror("\n[Erro] Não foi possivel configurar o sina SIGALRM \n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "\n[Servidor] Sinal SIGALRM configurado \n");
	
        
        //SERVIDOR
	//Verifica se servidor ja existe (ACCESS)        
        if (access(SERVER_FIFO, F_OK) == 0){
		perror("\n[Erro] Servidor ja existe!\n");
		exit(1);
	}
		
	//Cria fifo do servidor (MKFIFO)

	res = mkfifo(SERVER_FIFO, 0777);
	if (res == -1) {
		perror("\n[Erro] Erro no Mkfifo do servidor \n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "\n[Servidor] FIFO do servidor criado \n");
        
	//Passa o processo para background criando um processo filho e terminando o pai
	
	fork();	
	
	//Abre servidor (OPEN - O_RDWR)	
	fd_servidor = open(SERVER_FIFO, O_RDWR);  /* bloqueante */
	if (fd_servidor == -1) {
		perror("\n[Erro] Erro ao abrir o FIFO do servidor (RDWR/blocking) \n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "\n[Servidor] FIFO aberto para leitura! \n");

	
	while (1) { 

                //Lê mensagem
		res = read(fd_servidor, &msg, sizeof(msg));
		if (res < sizeof(msg)) 
                {
			fprintf(stderr, "\n[Erro] Recebido comando incompleto [bytes lidos: %d]", res);
		}
		else
			fprintf(stderr, "\n[Servidor] Comando -> %s \n", msg.comando);

                
                //Processo comandos
		comandos(&msg);
	} 

	running = 0;
	close(fd_servidor);
	unlink(SERVER_FIFO);
	return 0;
}
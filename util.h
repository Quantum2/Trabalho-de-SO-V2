#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/* nome do FIFO do servidor */
#define SERVER_FIFO "/tmp/serv_fifo"

/* nome do FIFO cada cliente. P %d ser� substitu�do pelo PID com sprintf */
#define CLIENT_FIFO "/tmp/cli_%d"

#define MAX 10
#define TAM_MAX 150

/* estrutura da mensagem correspondente a ligação entre cliente e servidor*/
typedef struct {
	int pid_cliente;
	char comando[TAM_MAX];
	int estado;
} MSG;

#define AGUARDA_LOGIN 1
#define AGUARDA_JOGO 2
#define A_JOGAR 3

typedef struct jogador{
	int saude, ataque, defesa, X, Y;
	float peso;
	int pid_cliente;
	char nome[TAM_MAX];
	char pass[TAM_MAX];
	int vitorias;
	int aJogar;
	int admin;
	struct jogador *Proximo;
}Jogador;

typedef struct jogadorLogout{
	char nome[TAM_MAX];
	char pass[TAM_MAX];
	int vitorias;
	struct jogadorLogout *Proximo;
}JogadorLogout;

/*--------------------------------------OBJECTOS-------------------------------*/

typedef struct monstro{
	char nome[100];
	int fataque, fdefesa, saude, existencia, quantidade; //existencia 1- existe - 0 - n existe
	bool agressivo; //true - agressivo, false-passivo
	bool hiper; //true - irrequieto , false - quieto
}Monstro;

typedef struct objecto{
	char nome[TAM_MAX];
	int raridade, fataque, fdefesa, musos, existencia; //existencia 1- existe - 0 - n existe
	// se maximo de usos, não tiver maximo, então musos=0; se não for para ser utilizada fica com -1;
	float peso;
}Objecto;

typedef struct sala{
    int num;
    int portaN, portaS, portaO, portaE; 
    Monstro mons[2];
    Objecto obj[5];
}Sala;

typedef struct labirinto{
	Sala s[MAX][MAX];
	Jogador j;
	int Dificuldade;
        int iX;
        int iY;
        char criado[TAM_MAX];
}Labirinto;

Jogador *jogadores = NULL, *auxJogadores = NULL;
int jogoCriado = 0,jogoDecorrer = 0,totalJogadores = 0;

void manda_mensagem_todos(char *mensagem,int pidNaoRecebe,int estado);
void actualizaEstado(int pid,int estado);
void escreveFifo(char *fifo,MSG string);
int geraNumAletorio(int min,int max);
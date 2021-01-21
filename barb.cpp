#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>						// _beginthreadex() e _endthreadex()
#include <conio.h>							// _getch
#include <cstdlib>

/* #include "CheckForError.h" */

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);	// Casting para terceiro e sexto parâmetros da função
                                                    // _beginthreadex
typedef unsigned *CAST_LPDWORD;

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY

#define	ESC				0x1B			// Tecla para encerrar o programa
#define N_CLIENTS		4			// Número de clientes
#define N_BARBS		    3			// Número de barbeiros
#define N_CASHIERS		1			// Número de caixas
#define N_CHAIRS        7           // Número de cadeiras (4 de espera e 3 de barbear)

DWORD WINAPI Barber(int);		// Thread´representando o babrbeiro
DWORD WINAPI Client(int);		// Thread representando o cliente

void FazABarbaDoCliente(int);		// Função que simula o ato de fazer a barba
void TerminaABarbaDoCliente(int);		// Função que simula o ato de fazer a barba
void TemABarbaFeita(int);			// Função que simula o ato de ter a barba feita

int nTecla;							// Variável que armazena a tecla digitada para sair
int client_counter = 0;
int id_cliente;                     // Identificador do cliente
int payers = 0; // numero de clientes a pagar

HANDLE look;  // mutex que controla acesso a variaveis globais
HANDLE chair; // semaforo que controla acesso a cadeira dos barbeiros
HANDLE wake; // semaforo que sinaliza para algum barbeiro acordar
HANDLE leave_chair; // semaforo que sinaliza que um cliente deixou a cadeira de um barbeiro
HANDLE end; // semaforo que sinaliza que o barbeiro terminou de barbear
HANDLE end_pay; // semaforo que indica que o babeiro caixa terminou de receber o pagamento

// THREAD PRIMÁRIA
int main(){

	HANDLE hThreads[N_CLIENTS + N_BARBS];       // N clientes mais o barbeiro
	DWORD dwIdBarbeiro, dwIdCliente;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	int i;

	// Obtém um handle para a saída da console

	// Cria objetos de sincronização

    look = CreateMutex(NULL, FALSE, NULL);
    chair = CreateSemaphore(NULL, N_BARBS, N_BARBS, NULL); // deve ser inicializado com o numero de barbeiros (para cada barb. ha uma cadeira)
    wake = CreateSemaphore(NULL, 0, N_BARBS, NULL); // deve ser inicializado em zero para que nenhum barbeiro trabalho sem necessidade
    leave_chair = CreateSemaphore(NULL, 0, N_BARBS, NULL);
    end = CreateSemaphore(NULL, 0, N_BARBS, NULL);
    end_pay = CreateSemaphore(NULL, 0, N_BARBS, NULL);

	// Criação de threads
	for (i=0; i < N_CLIENTS; ++i) {
		hThreads[i] = (HANDLE) _beginthreadex(
						       NULL,
							   0,
							   (CAST_FUNCTION) Client,	
							   (LPVOID)(INT_PTR)i,
							   0,								
							   (CAST_LPDWORD)&dwIdCliente);		
		/* SetConsoleTextAttribute(hOut, WHITE); */
		if (hThreads[i] != (HANDLE) -1L)
			printf("Thread Cliente %d criada com Id=%0x\n", i, dwIdCliente);
		else {
			printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
			exit(0);
		}
	}//for

	for (i=0; i < N_BARBS; ++i) {
		hThreads[i + N_CLIENTS] = (HANDLE) _beginthreadex(
						       NULL,
							   0,
							   (CAST_FUNCTION) Barber,	//Casting necessário
							   (LPVOID)(INT_PTR)i,
							   0,								
							   (CAST_LPDWORD)&dwIdCliente);		//Casting necessário
		/* SetConsoleTextAttribute(hOut, WHITE); */
		if (hThreads[i] != (HANDLE) -1L)
			printf("Thread Barbeiro %d criada com Id=%0x\n", i, dwIdCliente);
		else {
			printf("Erro na criacao da thread Cliente! N = %d Codigo = %d\n", i, errno);
			exit(0);
		}
	}//for

	// Leitura do teclado
	do {
		nTecla = _getch();
	} while (nTecla != ESC);
	
	// Aguarda término das threads homens e mulheres
	dwRet = WaitForMultipleObjects(N_CLIENTS+N_BARBS,hThreads,TRUE,INFINITE);
	/* CheckForError(dwRet==WAIT_OBJECT_0); */
	
	// Fecha todos os handles de objetos do kernel
	for (int i=0; i<N_CLIENTS+N_BARBS; ++i)
		CloseHandle(hThreads[i]);
	//for

	// Fecha os handles dos objetos de sincronização

    CloseHandle(look);
    CloseHandle(chair);
    CloseHandle(wake);
    CloseHandle(leave_chair);
    CloseHandle(end);
    CloseHandle(end_pay);

	return EXIT_SUCCESS;
	
}//main

DWORD WINAPI Client(int i) {

	DWORD dwStatus;
	BOOL bStatus;

	do {
        
		// Verifica se há lugar na barbearia
        WaitForSingleObject(look, INFINITE);
		if (client_counter == N_CHAIRS){
		    printf("Cliente %d encontrou a barbearia cheia e foi embora\n", i);
            ReleaseMutex(look);
			Sleep(2000);
			continue;
		}

		// Cliente entra na barbearia
		client_counter++;
        printf("Cliente %d entrou na barbearia...\n", i);
        ReleaseMutex(look);

        WaitForSingleObject(chair, INFINITE); // Cliente aguarda lugar em uma das cadeiras
        printf("Cliente %i se senta em uma cadeira de barbeiro\n\n", i);

        ReleaseSemaphore(wake, 1, NULL); // Cliente acorda um barbeiro
        printf("Cliente %i acorda um barbeiro\n\n", i);

        WaitForSingleObject(end, INFINITE); // Cliente espera barbeiro terminar seu corte
        ReleaseSemaphore(leave_chair, 1, NULL); // Cliente sinaliza que levantou da cadeira
        printf("Cliente %i levanta de cadeira\n\n", i);

        WaitForSingleObject(look, INFINITE); // Cliente entra para fila de pagamento
        printf("Cliente %i declara que quer pagar\n\n", i);
        payers++;
        ReleaseMutex(look);

        printf("Cliente %i espera para pagar em uma fila de %i clientes\n\n", i, payers);
        WaitForSingleObject(end_pay, INFINITE); // Cliente aguarda pagamento ser recebido
        printf("Cliente %i pagou\n\n", i);

        WaitForSingleObject(look, INFINITE); // Cliente sai da barbearia
        client_counter--;
        printf("Cliente %i sai da barbearia\n\n", i); 
        ReleaseMutex(look);

		Sleep(3000);

	} while (nTecla != ESC);

	printf("Thread cliente %d encerrando execucao...\n", i);
	_endthreadex(0);
	return(0);
}//ThreadCliente

DWORD WINAPI Barber(int i){

	DWORD dwStatus;
	BOOL bStatus;

    bool cashier = (i == 0);

    do{

        if(cashier){ 
            WaitForSingleObject(look, INFINITE);
            if(payers != 0){
                printf("Barbeiro caixa recebendo pagamento de %i clientes\n\n", payers);
                ReleaseSemaphore(end_pay, payers, NULL);
                payers = 0;
            }
            ReleaseMutex(look);
        }

        DWORD status = WaitForSingleObject(wake, 2000); // Barbeiro aguarda cliente o acordar
        
        if(status == WAIT_TIMEOUT){ // Trata o caso onde a solucao pode apresentar deadlock (babeiro caixa dorme com a fila cheia e nenhum cliente o desperta)

            if(cashier){  // Se for o caixa, ele recebe pagamento da fila e pula a interacao do while
                WaitForSingleObject(look, INFINITE);
                printf("Alarme do barbeiro caixa desperta! Ufa, quase tivemos um deadlock.\n\n");
                if(payers != 0){
                    printf("Barbeiro caixa recebendo pagamento de %i clientes\n\n", payers);
                    ReleaseSemaphore(end_pay, payers, NULL);
                    payers = 0;
                }
                ReleaseMutex(look);
                continue;

            }else{ 
                WaitForSingleObject(wake, INFINITE); // No caso (muito improvavel) de um barbeiro cujo alarme desperta nao for o caixa, ele vola a tirar seu cochilo
            }
        }

        FazABarbaDoCliente(i); // Faz a barba do cliente
        TerminaABarbaDoCliente(i);

        if(cashier){ 
            WaitForSingleObject(look, INFINITE);
            if(payers != 0){
                printf("Barbeiro caixa recebendo pagamento de %i clientes\n\n", payers);
                ReleaseSemaphore(end_pay, payers, NULL);
                payers = 0;
            }
            ReleaseMutex(look);
        }

        ReleaseSemaphore(end, 1, NULL); // Sinaliza que terminou

        WaitForSingleObject(leave_chair, INFINITE); // Espera cliente levantar da cadeira

        if(cashier){ 
            WaitForSingleObject(look, INFINITE);
            if(payers != 0){
                printf("Barbeiro caixa recebendo pagamento de %i clientes\n\n", payers);
                ReleaseSemaphore(end_pay, payers, NULL);
                payers = 0;
            }
            ReleaseMutex(look);
        }

        ReleaseSemaphore(chair, 1, NULL); // Sinaliza que cadeira esta livre
        Sleep(3000);

    }while(nTecla!=ESC);

	printf("Thread barbeiro encerrando execucao...\n");
	_endthreadex(0);
	return(0);
}

void FazABarbaDoCliente(int id) {

    int time = ((rand() % 10) + 1) * 2000; //cada barba demora de 2 a 20 segundos
	printf("Barbeiro %i fazendo barba em %i segundos\n", id, (time/1000));
	Sleep(time);
	return;
}

void TerminaABarbaDoCliente(int b_id) {

	printf("Barbeiro %i terminou barba...\n\n", b_id);
	Sleep(1000);
	return;
}

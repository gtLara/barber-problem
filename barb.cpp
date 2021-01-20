#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>						// _beginthreadex() e _endthreadex()
#include <conio.h>							// _getch

/* #include "CheckForError.h" */

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);	// Casting para terceiro e sexto parâmetros da função
                                                    // _beginthreadex
typedef unsigned *CAST_LPDWORD;

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY

#define	ESC				0x1B			// Tecla para encerrar o programa
#define N_CLIENTS		20			// Número de clientes
#define N_BARBS		    4			// Número de barbeiros
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

HANDLE look;  
HANDLE waiting_room;  
HANDLE chair;
HANDLE wake;
HANDLE work;
HANDLE leave_chair;
HANDLE end;
HANDLE pay;
HANDLE end_pay;

// THREAD PRIMÁRIA
int main(){

	HANDLE hThreads[N_CLIENTS+N_BARBS];       // N clientes mais o barbeiro
	DWORD dwIdBarbeiro, dwIdCliente;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	int i;

	// Obtém um handle para a saída da console
	/* hOut = GetStdHandle(STD_OUTPUT_HANDLE); */
	/* if (hOut == INVALID_HANDLE_VALUE) */
	/* 	printf("Erro ao obter handle para a saída da console\n"); */

	// Cria objetos de sincronização

    look = CreateMutex(NULL, FALSE, NULL);
    pay = CreateSemaphore(NULL, 0, 1, NULL);
    /* waiting_room = CreateSemaphore(NULL, waiting_room_capacity, waiting_room_capacity, NULL); */
    chair = CreateSemaphore(NULL, N_BARBS, N_BARBS, NULL);
    wake = CreateSemaphore(NULL, 0, N_BARBS, NULL); // sera que maximo deve ser n_barb mesmo?
    work = CreateSemaphore(NULL, N_BARBS, N_BARBS, NULL);
    leave_chair = CreateSemaphore(NULL, 0, N_BARBS, NULL);
    end = CreateSemaphore(NULL, 0, N_BARBS, NULL);
    end_pay = CreateSemaphore(NULL, 0, N_BARBS, NULL);

	// Criação de threads
	// Note que _beginthreadex() retorna -1L em caso de erro
	for (i=0; i < N_CLIENTS; ++i) {
		hThreads[i] = (HANDLE) _beginthreadex(
						       NULL,
							   0,
							   (CAST_FUNCTION) Client,	//Casting necessário
							   (LPVOID)(INT_PTR)i,
							   0,								
							   (CAST_LPDWORD)&dwIdCliente);		//Casting necessário
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
    CloseHandle(pay);
    /* CloseHandle(waiting_room); */
    CloseHandle(chair);
    CloseHandle(wake);
    CloseHandle(work);
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
			/* SetConsoleTextAttribute(hOut, HLRED); */
		    printf("Cliente %d encontrou a barbearia cheia e foi embora\n", i);
            ReleaseMutex(look);
			Sleep(2000);
			continue;
			/* SetConsoleTextAttribute(hOut, WHITE); */
		}
		// Cliente entra na barbearia
		client_counter++;
        printf("Cliente %d entrou na barbearia...\n", i);
        ReleaseMutex(look);

        /* WaitForSingleObject(waiting_room, INFINITE); */
        /* printf("Cliente %i entra em sala de espera"); */
        WaitForSingleObject(chair, INFINITE);
        /* ReleaseSemaphore(waiting_room, 1, NULL); */
        printf("Cliente %i se senta em uma cadeira de barbeiro\n\n", i);
        ReleaseSemaphore(wake, 1, NULL);
        printf("Cliente %i acorda um barbeiro\n\n", i);
        WaitForSingleObject(end, INFINITE);
        ReleaseSemaphore(leave_chair, 1, NULL);
        printf("Cliente %i levanta de cadeira\n\n", i);
        ReleaseSemaphore(pay, 1, NULL);
        printf("Cliente %i paga\n", i);
        WaitForSingleObject(end_pay, INFINITE);
        printf("Cliente %i sai da barbearia\n\n", i);

        WaitForSingleObject(look, INFINITE);
        client_counter--;
        ReleaseMutex(look);

		/* // Cliente aguarda sua vez */
        /* ReleaseSemaphore(hAguardaCliente, 1, NULL); */
		/* // Cliente acorda o barbeiro */
		/* id_cliente = i; */
        /* WaitForSingleObject(hBarbeiroLivre, INFINITE); */
		/* // Cliente tem sua barba feita pelo barbeiro */
		/* TemABarbaFeita(i); */
        /* WaitForSingleObject(hBarbeiroTerminou, INFINITE); */
		/* // Cliente sai da barbearia */

        /* WaitForSingleObject(hMutex, NULL); */
		/* n_clientes--; */
        /* ReleaseMutex(hMutex); */

		/* SetConsoleTextAttribute(hOut, WHITE); */
		/* printf("Cliente %d saindo da barbearia...\n", i); */

		Sleep(100);

	} while (nTecla != ESC);

	/* SetConsoleTextAttribute(hOut, WHITE); */
	printf("Thread cliente %d encerrando execucao...\n", i);
	_endthreadex(0);
	return(0);
}//ThreadCliente

DWORD WINAPI Barber(int i) {

	DWORD dwStatus;
	BOOL bStatus;

    bool cashier = (i == 0);

    do{
        /* if(cashier){ */
        /*     DWORD non_timeout = WaitForSingleObject(pay, INFINITE); */
        /*     if(non_timeout){ */
        /*         printf("Barbeiro %i recebe pagamento \n\n", i); */
        /*         ReleaseSemaphore(end_pay, 1, NULL); */
        /*     } */

        /* ReleaseSemaphore(chair, 1, NULL); */
        /* } */

        WaitForSingleObject(wake, INFINITE);
        WaitForSingleObject(work, INFINITE); // investigar necessidade desse semaforo. pode ser desnecessario
        FazABarbaDoCliente(i);
        /* printf("Barbeiro %i corta cabelo \n\n", i); */
        ReleaseSemaphore(work, 1, NULL);
        TerminaABarbaDoCliente(i);
        ReleaseSemaphore(end, 1, NULL);
        WaitForSingleObject(leave_chair, INFINITE);

        /* if(cashier){ */
            WaitForSingleObject(pay, INFINITE);
            printf("Barbeiro %i recebe pagamento \n\n", i);
            ReleaseSemaphore(end_pay, 1, NULL);

        ReleaseSemaphore(chair, 1, NULL);
        /* } */
    Sleep(100);
    }while(nTecla!=ESC);

	/* SetConsoleTextAttribute(hOut, HLGREEN); */
	printf("Thread barbeiro encerrando execucao...\n");
	_endthreadex(0);
	return(0);
}

void FazABarbaDoCliente(int id) {

	/* SetConsoleTextAttribute(hOut, HLGREEN); */
	printf("Barbeiro %i fazendo barba\n", id);
	Sleep(10);
	return;
}

void TerminaABarbaDoCliente(int b_id) {

	/* SetConsoleTextAttribute(hOut, HLGREEN); */
	printf("Barbeiro %i terminou barba...\n\n", b_id);
	Sleep(10);
	return;
}

/* void TemABarbaFeita(int c_id, int b_id) { */

/* 	SetConsoleTextAttribute(hOut, HLGREEN); */
/* 	printf("Cliente %d tem sua barba feita pelo barbeiro %i...\n\n", c_id, b_id); */
/* 	Sleep(10); */
/* 	return; */
/* } */

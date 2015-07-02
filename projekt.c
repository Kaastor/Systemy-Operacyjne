#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#define SIZE 512
#define KROPKA 46

//ZMIENNE
FILE *plik;
int wybor = 0;
char bufor_tekstowy[SIZE], *eof = bufor_tekstowy, wynik, tmp, znak;
int pfd1_2[2], pfd2_3[2], pfd3_1_synchro[2], pfd1_3_wybor[2];
int synchro = 0, ilosc_znakow, i;
int fd_write, fd_read;
char fifo_name[100];
int pid_reset;
int stop = 0;
//STRUKTURA KOMUNIKATU
struct msg {
	pid_t pid;
	int sygnal;
}msg;

//**********************KOLEJKI FIFO**********************//
int mk_fifo_name(pid_t pid, char* name, size_t name_max) //funkcja tworzaca nazwe kolejki fifo "fifoPID"
{
	snprintf(name, name_max, "fifo%ld", (long)pid);
	return 1;
}
void fifo_send_id(int id) //funkcja wysylajaca do kolejki fifo o nazwie fifoid, id jest argumentem funkcji
{
	mk_fifo_name(id, fifo_name, sizeof(fifo_name)); //tworzenie nazwy kolejki 
	fd_write = open(fifo_name, O_WRONLY); //otwarcie do zapisu kolejki fifo
	write(fd_write, &msg, sizeof(msg)); //zapisanie numeru sygnalu do kolejki
	close(fd_write); //zamkniecie fifo
}
void fifo_send_pm()
{
	fd_write = open("fifo_PM", O_WRONLY); //otwarcie do zapisu kolejki fifo procesu macierzystego
	write(fd_write, &msg, sizeof(msg)); //zapisanie numeru sygnalu do kolejki
	close(fd_write); //zamkniecie fifo
}
void fifo_read_pm()
{
	fd_read = open("fifo_PM", O_RDONLY); //otwarcie do odczytu kolejki fifo procesu macierzystego
	read(fd_read, &msg, sizeof(msg)); //odczytanie numeru sygnalu z kolejki 
	close(fd_read); //zamkniecie fifo
}
void fifo_name_read()
{
	fd_read = open(fifo_name, O_RDONLY); //otwarcie do odczytu kolejki fifo
	read(fd_read, &msg, sizeof(msg)); //odczytanie numeru sygnalu z kolejki 
	close(fd_read); //zamkniecie fifo
}

//**********************SYGNALY**********************///
void sig_term_P1(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t P1 = getpid();//pobranie pid procesu 1
	if (info->si_pid == P1)//weryfikacja pid procesu wysylajacego syngal
	{
		unlink(fifo_name); //usniecie kolejki
		_exit(2);//zakonczenie procesu 1
	}
}

void sig_term_P2(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t P2 = getpid();//pobranie pid procesu 2
	if (info->si_pid == P2)//weryfikacja pid procesu wysylajacego syngal
	{
		unlink(fifo_name); //usniecie kolejki
		_exit(2);//zakonczenie procesu 2
	}
}

void sig_term_PM(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t PM = getpid(); //pobranie pid procesu macierzystego
	if (info->si_pid == PM)//weryfikacja pid procesu wysylajacego syngal
	{
		unlink("fifo_PM"); //usniecie kolejki
		_exit(2);//zakonczenie procesu macierzystego
	}
}

void sig_tstp_P1(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t PM = getppid();//pobranie pid procesu macierzystego
	if (info->si_pid == PM || info->si_pid == getpid())//weryfikacja pid procesu wysylajacego syngal
	{
		stop = 1;//zmienna sygnalizujaca otrzymanie sygnalu 20
		while (stop == 1){ ; }//petla nieskonczona dopoki zmienna stop == 1
	}
}

void sig_tstp_P2(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t P1 = getppid() + 1;//pobranie pid procesu 1
	if (info->si_pid == P1 || info->si_pid == getpid())//weryfikacja pid procesu wysylajacego syngal
	{
		stop = 1;//zmienna sygnalizujaca otrzymanie sygnalu 20
		while (stop == 1){ ; }//petla nieskonczona dopoki zmienna stop == 1
	}
}

void sig_cont_P1(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t PM = getppid();//pobranie pid procesu macierzystego
	if (info->si_pid == PM)//weryfikacja pid procesu wysylajacego syngal
	{
		stop = 0;//odblokowanie procesu macierzystego
	}
}

void sig_cont_P2(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t PM = getppid() + 1;//pobranie pid procesu 1
	if (info->si_pid == PM)//weryfikacja pid procesu wysylajacego syngal
	{
		stop = 0;//odblokowanie procesu 2
	}
}

void sig_kons_P3(int nr_syg)//funkcja obslugi sygnalu 18 dla procesu p1
{
	msg.sygnal = nr_syg;
	kill(getppid(), 12); //wyslanie sygnalu SIGUSR2 do procesu macierzystego
	fifo_send_pm();//zapis do kolejki fifo procesu macierzystego odebranego numeru sygnalu
}

void sig_usr2_PM(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t PM = getpid() + 3;//pobranie pid procesu 3
	if (info->si_pid == PM)//weryfikacja pid procesu wysylajacego syngal
	{
		pid_t PM = getpid(); //pobranie pid procesu macierzystego
		int P[3] = { PM + 1, PM + 2, PM + 3 };//tablica identyfikatorow procesow
		fifo_read_pm(); //odebranie komunikatu z fifo PM
		if (msg.sygnal == 18)
			kill(P[0], 18); //wzbudzenie procesu pierwszego (SIGCONT)
		kill(P[0], 12); //"kopniecie" P1 zeby odebral nr sygnalu
		fifo_send_id(P[0]); //zapis numeru sygnalu do fifo procesu1
		fifo_send_id(P[1]); //zapis numeru sygnalu do fifo procesu2
		fifo_send_id(P[2]); //zapis numeru sygnalu do fifo procesu3
		if (msg.sygnal == 15)
			raise(15); //wyslanie sygnalu SIGTERM do biezacego procesu
	}
}

void sig_usr2_P1(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t PM = getppid();//pobranie pid procesu macierzystego
	if (info->si_pid == PM)//weryfikacja pid procesu wysylajacego syngal
	{
		int pid = pid_reset; //pobranie pid procesu1
		fifo_name_read();    //odczytanie numeru sygnalu z fifo procesu1
		if (msg.sygnal == 15)
		{
			if (wybor == 2)
			{
				while (eof != NULL) //dokonczenie odczytywania pliku
				{
					read(pfd3_1_synchro[0], &synchro, sizeof(synchro)); //synchronizacja z P3, oczekiwanie na obliczenie ilosci znakow przez proces3
					write(pfd1_2[1], bufor_tekstowy, sizeof(bufor_tekstowy)); //zapis pobranej linii do pipe'a
					printf("%s", bufor_tekstowy);  			       //wypisanie zawartosci bufora
					eof = fgets(bufor_tekstowy, sizeof(bufor_tekstowy), plik); //pobranie linii z pliku
				}
				read(pfd3_1_synchro[0], &synchro, sizeof(synchro)); //synchronizacja z P3, oczekiwanie na obliczenie ilosci znakow przez proces3
			}
		}
		else if (msg.sygnal == 18)
			kill(++pid, 18); //wyslanie sygnalu SIGCONT do procesu2
		pid = pid_reset;
		kill(++pid, 12); //wyslanie sygnalu SIGUSR2 do procesu2
		if (msg.sygnal == 15) raise(15); //wyslanie sygnalu SIGTERM do biezacego procesu
		else if (msg.sygnal == 20) raise(20); //wyslanie sygnalu SIGTSTP do biezacego procesu
	}
}

void sig_usr2_P2(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t P1 = getppid() + 1;//pobranie pid procesu 1
	if (info->si_pid == P1)//weryfikacja pid procesu wysylajacego syngal
	{
		int pid = pid_reset; //pobranie pid procesu2
		fifo_name_read(); //odczytanie numeru sygnalu z fifo procesu2
		kill(++pid, 12); //wyslanie sygnalu SIGUSR2 do procesu3
		if (msg.sygnal == 15) raise(15); ////wyslanie sygnalu SIGTERM do biezacego procesu
		else if (msg.sygnal == 20) raise(20); //wyslanie sygnalu SIGTSTP do biezacego procesu
	}
}

void sig_usr2_P3(int nr_syg, siginfo_t *info, void *context)//funkcja obslugi sygnalu 18 dla procesu p1
{
	pid_t P2 = getppid() + 2;//pobranie pid procesu 2
	if (info->si_pid == P2)//weryfikacja pid procesu wysylajacego syngal
	{
		fifo_name_read(); //odczytanie numeru sygnalu z fifo procesu3
		if (msg.sygnal == 15)
		{
			unlink(fifo_name); //usniecie kolejki
			_exit(2); //zakonczenie procesu3
		}
		else if (msg.sygnal == 20) raise(19); //wyslanie sygnalu SIGSTP do biezacego procesu
	}
}
//**********************////**********************//

//RESET BUFORA
void reset_bufor(void)
{
	memset(bufor_tekstowy, 0, sizeof(bufor_tekstowy));
}

//MENU PROGRAMU
void menu(void)
{
	printf("Menu programu: \n");
	printf("1. Rozpocznij wprowadzanie ze standardowego wejscia.\n");
	printf("2. Rozpocznij odczyt z pliku.\n");
	printf("Podaj liczbe: ");
	scanf("%d", &wybor); //pobranie z konsoli wyboru
	write(pfd1_3_wybor[1], &wybor, sizeof(wybor)); //przeslanie wartosci wyboru do P3
	getchar(); //czyszczenie bufora klawiatury
}

//FUNKCJA OBSLUGI STDIN (1)
void obsluga_1_P1(void)
{
	while (bufor_tekstowy[0] != KROPKA)//pobieranie linii z konsoli dopoki nie wprowadzono znaku kropki
	{
		fgets(bufor_tekstowy, sizeof(bufor_tekstowy), stdin);//pobranie linii i zapis do bufora 
		write(pfd1_2[1], bufor_tekstowy, sizeof(bufor_tekstowy));//zapis bufora do pipe'a
	}
}

//FUNKCJA OBSLUGI PLIKU (2)
void obsluga_2_P1(void)
{
	eof = fgets(bufor_tekstowy, sizeof(bufor_tekstowy), plik);//pobranie linii z pliku
	while (eof != NULL)//pobieranie kolejnych linni z pliku
	{
		write(pfd1_2[1], bufor_tekstowy, sizeof(bufor_tekstowy));//zapis pobranej linii do pipe'a
		printf("%s", bufor_tekstowy); //wypisanie zawartosci bufora
		read(pfd3_1_synchro[0], &synchro, sizeof(synchro)); //synchronizacja z P3, oczekiwanie na obliczenie ilosci znakow przez proces3
		eof = fgets(bufor_tekstowy, sizeof(bufor_tekstowy), plik); //pobranie linii z pliku
	}
	bufor_tekstowy[0] = KROPKA;
	write(pfd1_2[1], bufor_tekstowy, sizeof(bufor_tekstowy)); //po odczycie pliku zapis '.' do pipe'a P2, przerwanie petli procesu2
}

//MAIN
int main(void)
{
	pid_t proces1, proces2, proces3;
	//STWORZENIE LACZY
	pipe(pfd1_2); //PIPE P1 -> P2
	pipe(pfd2_3); //PIPE P2 -> P3
	pipe(pfd3_1_synchro); //PIPE P3->P1
	pipe(pfd1_3_wybor); //PIPE P1->P3
	//FIFO PM
	mkfifo("fifo_PM", 0666);//stworzenie fifo procesu macierzystego
	//SYGNALY - DEF STRUKTUR
	sigset_t P1_set, P2_set, P3_set, PM_set;//stworzenie zbiorow masek
	// zbiory sygnalow syg
	sigfillset(&P1_set); //wypelnienie maski wszystkimi sygnalami
	sigdelset(&P1_set, 12); //usuniecie z maski sygnalu 12
	sigdelset(&P1_set, 18); //usuniecie z maski sygnalu 18
	sigdelset(&P1_set, 15); //usuniecie z maski sygnalu 15
	sigdelset(&P1_set, 20); //usuniecie z maski sygnalu 20
	sigfillset(&P2_set); //wypelnienie maski wszystkimi sygnalami
	sigdelset(&P2_set, 12); //usuniecie z maski sygnalu 12
	sigdelset(&P2_set, 18); //usuniecie z maski sygnalu 18
	sigdelset(&P2_set, 15); //usuniecie z maski sygnalu 15
	sigdelset(&P2_set, 20); //usuniecie z maski sygnalu 20
	sigfillset(&P3_set); //wypelnienie maski wszystkimi sygnalami
	sigdelset(&P3_set, 12); //usuniecie z maski sygnalu 12
	sigdelset(&P3_set, 18); //usuniecie z maski sygnalu 18
	sigdelset(&P3_set, 15); //usuniecie z maski sygnalu 15
	sigdelset(&P3_set, 20); //usuniecie z maski sygnalu 20
	sigfillset(&PM_set); //wypelnienie maski wszystkimi sygnalami
	sigdelset(&PM_set, 12); //usuniecie z maski sygnalu 12
	sigdelset(&PM_set, 15); //usuniecie z maski sygnalu 15
	// SYGNALY - OBSLUGA
	static struct sigaction P3_KONS;//utworzenie struktury obslugi sygnalow procesu3 wysylanych z konsoli
	P3_KONS.sa_handler = sig_kons_P3;//przypisanie nowej funkcji obslugi sygnalow
	P3_KONS.sa_flags = SA_RESTART; //ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	static struct sigaction PM_USR2;//utworzenie struktury obslugi sygnalow procesu3 wysylanych z konsoli
	PM_USR2.sa_sigaction = sig_usr2_PM;//przypisanie nowej funkcji obslugi sygnalow
	PM_USR2.sa_flags = SA_SIGINFO;//ustawienie flagi pozwalajacej na uzyskanie pid procesu wysylajacego syg
	static struct sigaction P1_USR2;//utworzenie struktury obslugi sygnalu USR2 procesu1 
	P1_USR2.sa_sigaction = sig_usr2_P1;//przypisanie nowej funkcji obslugi sygnalow
	P1_USR2.sa_flags = SA_RESTART + SA_SIGINFO; //ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu oraz 
	static struct sigaction P2_USR2;//utworzenie struktury obslugi sygnalu USR2 procesu2 
	P2_USR2.sa_sigaction = sig_usr2_P2;//przypisanie nowej funkcji obslugi sygnalow
	P2_USR2.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	static struct sigaction P3_USR2;//utworzenie struktury obslugi sygnalu USR2 procesu3 
	P3_USR2.sa_sigaction = sig_usr2_P3;//przypisanie nowej funkcji obslugi sygnalow
	P3_USR2.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	///SIGTERM
	static struct sigaction P1_TERM;//utworzenie struktury obslugi sygnalu TERM procesu1 
	P1_TERM.sa_sigaction = sig_term_P1;//przypisanie nowej funkcji obslugi sygnalow
	P1_TERM.sa_flags = SA_RESTART + SA_SIGINFO; //ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	static struct sigaction P2_TERM;//utworzenie struktury obslugi sygnalu TERM procesu2 
	P2_TERM.sa_sigaction = sig_term_P2;//przypisanie nowej funkcji obslugi sygnalow
	P2_TERM.sa_flags = SA_RESTART + SA_SIGINFO; //ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	static struct sigaction PM_TERM;//utworzenie struktury obslugi sygnalu TERM procesu macierzystego
	PM_TERM.sa_sigaction = sig_term_PM;//przypisanie nowej funkcji obslugi sygnalow
	PM_TERM.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	//SIGTSTP
	static struct sigaction P1_TSTP;//utworzenie struktury obslugi sygnalu TSTP procesu1 
	P1_TSTP.sa_sigaction = sig_tstp_P1;//przypisanie nowej funkcji obslugi sygnalow
	P1_TSTP.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	static struct sigaction P2_TSTP;//utworzenie struktury obslugi sygnalu TSTP procesu2
	P2_TSTP.sa_sigaction = sig_tstp_P2;//przypisanie nowej funkcji obslugi sygnalow
	P2_TSTP.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	//SIGCONT 
	static struct sigaction P1_CONT;//utworzenie struktury obslugi sygnalu CONT procesu1 
	P1_CONT.sa_sigaction = sig_cont_P1;//przypisanie nowej funkcji obslugi sygnalow
	P1_CONT.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 
	static struct sigaction P2_CONT;//utworzenie struktury obslugi sygnalu CONT procesu2
	P2_CONT.sa_sigaction = sig_cont_P2;//przypisanie nowej funkcji obslugi sygnalow
	P2_CONT.sa_flags = SA_RESTART + SA_SIGINFO;//ustawienie flagi wznawiajacej dzialanie procesu od momentu otrzymania sygnalu 

	//PROCES 1
	proces1 = fork();//utworzenie procesu potomnego p1
	switch (proces1)
	{
	case 0:
		//STWORZENIE FIFO
		pid_reset = msg.pid = getpid();//pobranie pid procesu p1
		mk_fifo_name(msg.pid, fifo_name, sizeof(fifo_name)); //tworzenie nazwy fifo P1
		mkfifo(fifo_name, 0666); //tworzenie fifo o nazwie fifoPID
		//SYGNALY
		sigprocmask(SIG_BLOCK, &P1_set, NULL);//zablokowanie sygnalow ze zbioru P1_set
		sigaction(12, &P1_USR2, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 12
		sigaction(15, &P1_TERM, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 15
		sigaction(20, &P1_TSTP, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 20
		sigaction(18, &P1_CONT, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 18
		while (1)
		{
			menu(); //wyswietlenie menu programu
			if (wybor == 1)
				obsluga_1_P1();//obsluga wczytywania ciagow znakow z STDIN
			if (wybor == 2)
			{
				plik = fopen("plik_we.txt", "r");//otwarcie pliku zrodlowego
				obsluga_2_P1();//obsluga wczytywania ciagow znakow z pliku
				fclose(plik); //zamkniecie pliku
			}
			reset_bufor();	//czyszczenie bufora tekstowego				
		}
	}

	//PROCES 2
	proces2 = fork();//utworzenie procesu potomnego p2
	switch (proces2)
	{
	case 0:
		//STWORZENIE FIFO
		pid_reset = msg.pid = getpid(); //pobranie pid procesu p2
		mk_fifo_name(msg.pid, fifo_name, sizeof(fifo_name)); //tworzenie nazwy fifo P2
		mkfifo(fifo_name, 0666); //tworzenie fifo o nazwie fifoPID
		//SYGNALY
		sigprocmask(SIG_BLOCK, &P2_set, NULL);//zablokowanie sygnalow ze zbioru P2_set
		sigaction(12, &P2_USR2, NULL);//ustawienie nowej akcji zwiazanej z sygnalem 12
		sigaction(15, &P2_TERM, NULL);//ustawienie nowej akcji zwiazanej z sygnalem 15
		sigaction(20, &P2_TSTP, NULL);//ustawienie nowej akcji zwiazanej z sygnalem 20
		sigaction(18, &P2_CONT, NULL);//ustawienie nowej akcji zwiazanej z sygnalem 18
		while (1)
		{
			while (1)
			{
				read(pfd1_2[0], bufor_tekstowy, sizeof(bufor_tekstowy));//odczytanie z pipe'a wczytanego ciagu znakow od procesu1
				if (bufor_tekstowy[0] == KROPKA)
					ilosc_znakow = KROPKA;
				else
					ilosc_znakow = strlen(bufor_tekstowy) - 1; //obliczenie ilosci znakow
				write(pfd2_3[1], &ilosc_znakow, sizeof(ilosc_znakow));//zapis do pipe'a obliczonej ilosci znakow
				if (bufor_tekstowy[0] == KROPKA) break; // zakonczenie liczenia w przypadku kropki na STDIN lub konca pliku
			}
		}
	}

	//PROCES 3
	proces3 = fork();//utworzenie procesu potomnego p3
	switch (proces3)
	{
	case 0:
		//STWORZENIE FIFO
		pid_reset = msg.pid = getpid(); //pobranie pid procesu p3
		mk_fifo_name(msg.pid, fifo_name, sizeof(fifo_name)); //tworzenie nazwy fifo P3
		mkfifo(fifo_name, 0666); //tworzenie fifo o nazwie fifoPID
		//SYGNALY
		sigprocmask(SIG_BLOCK, &P3_set, NULL);//zablokowanie sygnalow ze zbioru P3_set
		sigaction(20, &P3_KONS, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 20
		sigaction(18, &P3_KONS, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 18
		sigaction(15, &P3_KONS, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 15
		sigaction(12, &P3_USR2, NULL); //ustawienie nowej akcji zwiazanej z sygnalem 12
		while (1)
		{
			read(pfd1_3_wybor[0], &wybor, sizeof(wybor)); //odczyt z pipe'a wybranego trybu pracy programu
			while (1)
			{
				read(pfd2_3[0], &ilosc_znakow, sizeof(ilosc_znakow)); //odczyt z pipe'a ilosci znakow obliczonej przez proces2
				if (ilosc_znakow == KROPKA) break;
				printf("%d\n", ilosc_znakow); //wyswietlenie ilosci znakow w konsoli
				if (wybor == 2) //jesli PLIK to synchronizacja z procesem1
					write(pfd3_1_synchro[1], &synchro, sizeof(synchro));//zezwolenie procesowi1 na pobranie kolejnej linii z pliku
			}
		}
	}

	//SYGNALY PM
	sigprocmask(SIG_BLOCK, &PM_set, NULL);//zablokowanie sygnalow ze zbioru PM_set
	sigaction(12, &PM_USR2, NULL);//ustawienie nowej akcji zwiazanej z sygnalem 12
	sigaction(15, &PM_TERM, NULL);//ustawienie nowej akcji zwiazanej z sygnalem 15
	while (1)
		pause();

	return 0;
}

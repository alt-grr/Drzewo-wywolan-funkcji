#include "parser.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#ifdef DEBUG
	#include <stdio.h>
#endif

/******************************************************************************
						Prywatne rzeczy modułu
*******************************************************************************/

/***************************
		Struktury
***************************/

// Lista możliwych stanów podczas parsowania - patrz funkcja 'preparsuj'
typedef enum {
	ST_KOD, ST_MAKRO, ST_KOMENTARZ_C, ST_KOMENTARZ_CPP, ST_STRING
} stan_t;

//	Lista makr
typedef struct {
	unsigned int liczba;
	char **nazwy;
} listamakr_t;

//	Opis pojedynczej funkcji / wywołania funkcji / wywołania makra
typedef struct {
	char *nazwa;
	typdrzewa_t typ;
	unsigned long znak_start; //początek od znaku
	unsigned long znak_stop;  //koniec na znaku
	unsigned long linia_start;//początek w linii
	unsigned long linia_stop; //koniec w linii
} funkcja_t;

//	Lista funkcji
typedef struct {
	unsigned int liczba;
	funkcja_t **funkcje;
} listafunkcji_t;




/***************************
	Obsługa listamakr_t
****************************/

//konstruktor
listamakr_t *
listamakr_create(void)
{
	listamakr_t *wsk = malloc(sizeof (listamakr_t));
	if (wsk == NULL)
		return NULL;

	wsk->liczba = 0;
	wsk->nazwy = NULL;
	return wsk;
}

//destruktor
void
listamakr_free(listamakr_t *makra)
{
	unsigned int i;
	if (makra->nazwy != NULL)
		for (i=0; i < makra->liczba; i++)
			if (makra->nazwy[i] != NULL)
				free(makra->nazwy[i]);

	free(makra);
}

//dodaje nowe makro "nazwa" do listy makr "makra"
listamakr_t *
listamakr_add(listamakr_t *makra, const char *nazwa)
{
	//alokowanie pamięci gdy to pierwsze makro
	if (makra->liczba == 0)
	{
		if ((makra->nazwy = malloc(sizeof (char*))) == NULL)
			return NULL;
		if ((makra->nazwy[0] = malloc((strlen(nazwa) + 1) * sizeof (char))) == NULL)
		{
			free(makra->nazwy);
			return NULL;
		}
	}
	//realokacja gdy to kolejne makro
	else
	{
		if ((makra->nazwy = realloc(makra->nazwy, (makra->liczba + 1) * sizeof (char*))) == NULL)
			return NULL;

		if ((makra->nazwy[makra->liczba] = malloc((strlen(nazwa) + 1) * sizeof (char))) == NULL)
			return NULL;
	}

	strcpy(makra->nazwy[makra->liczba++], nazwa);

	return makra;
}

#ifdef DEBUG
//drukuje listę makr
void
listamakr_print(FILE *wyj, const listamakr_t *makra)
{
	fprintf(wyj, "Znalezione makra:\n");

	unsigned int i;
	for (i=0; i < makra->liczba; i++)
		fprintf(wyj, "Makro %d: %s\n", i+1, makra->nazwy[i]);

	fprintf(wyj, "\n");
}

#endif


/***************************
	Obsługa funkcja_t
****************************/

//konstruktor
funkcja_t *
funkcja_create(const char *nazwa, const typdrzewa_t typ, const unsigned long znak_start, const unsigned long znak_stop,
	const unsigned long linia_start, const unsigned long linia_stop)
{
	//alokujemy pamięć
	funkcja_t *funkcja;
	if ((funkcja = malloc(sizeof (funkcja_t))) == NULL)
		return NULL;

	if ((funkcja->nazwa = malloc((strlen(nazwa) + 1) * sizeof (char))) == NULL)
	{
		free(funkcja);
		return NULL;
	}

	//wypełniamy danymi
	strcpy(funkcja->nazwa, nazwa);
	funkcja->typ = typ;
	funkcja->znak_start = znak_start;
	funkcja->znak_stop = znak_stop;
	funkcja->linia_start = linia_start;
	funkcja->linia_stop = linia_stop;

	return funkcja;
}

//destruktor
void
funkcja_free(funkcja_t *funkcja)
{
	free(funkcja->nazwa);
	free(funkcja);
}



/***************************
	Obsługa listafunkcji_t
****************************/

//konstruktor
listafunkcji_t *
listafunkcji_create(void)
{
	//alokujemy pamięć
	listafunkcji_t *listafunkcji;
	if ((listafunkcji = malloc(sizeof (listafunkcji_t))) == NULL)
		return NULL;

	//inicjalizacja struktury
	listafunkcji->liczba = 0;
	listafunkcji->funkcje = NULL;

	return listafunkcji;
}

//destruktor
void
listafunkcji_free(listafunkcji_t *listafunkcji)
{
	unsigned int i;
	for (i=0; i < listafunkcji->liczba; i++)
		if (listafunkcji->funkcje[i] != NULL)
			funkcja_free(listafunkcji->funkcje[i]);

	free(listafunkcji);
}

//podpina potomka do rodzica
listafunkcji_t *
listafunkcji_add(listafunkcji_t *rodzic, const funkcja_t *potomek)
{
	//alokacja pamięći jesli to pierwszy potomek
	if (rodzic->liczba == 0)
	{
		if ((rodzic->funkcje = malloc(sizeof (funkcja_t*))) == NULL)
			return NULL;
	}
	//robimy miejsce na kolejnego potomka
	else if (rodzic->liczba == UINT_MAX
				|| (rodzic->funkcje = realloc(rodzic->funkcje, (rodzic->liczba+1) * sizeof (funkcja_t*))) == NULL)
		return NULL;

	rodzic->funkcje[rodzic->liczba++] = (funkcja_t*)potomek;

	return rodzic;
}





/*********************************************
			Inne funkcje prywatne
**********************************************/

//zlicza liczbę linii pomiędzy wskazanymi miejscami w pliku
unsigned long
zlicz_linie(const kodplik_t *plik, const unsigned long znak_start, const unsigned long znak_stop)
{
	unsigned long i, linie = 1;
	for (i=znak_start; i <= znak_stop; i++)
		if (plik->kod[i] == '\n')
			linie++;
	return linie;
}

//parsuje nazwę makra
//"start" to wskaźnik do pierwszego znaku za '#'
//"ile" to liczba znaków z ilu składa się dana do parsowania dyrektywa
char *
parsuj_nazwe_makra(char *start, unsigned int ile)
{
	unsigned int i;          //licznik ale także informacja ile znaków wyciąć
	char *now_start = start;  //pozycja od której będziemy wycinać

	//wykrywamy początek nazwy
	for (i=0; i < ile; i++)
		if (isspace(start[i]))
			now_start++;
		else
			break;

	//wykrywamy koniec nazwy
	unsigned int nile = ile-i;
	for (i=0; i < nile; i++)
		if (now_start[i] == '(' || isspace(now_start[i]))
			break;

	//cofamy się, jęsli mamy w nazwie znak nowej linii żeby go nie nadpisać
	if (now_start[i-1] == '\n')
		i--;

	//tworzymy nowego stringa
	char *nowy = malloc((i+1) * sizeof (char));
	strncpy(nowy, now_start, i);
	nowy[i] = '\0';

	//sprzątanie treści pliku
	ile+=7;
	start-=6; //bo musimy też wyciąć 'define' z przodu
	for (i=0; i < ile; i++)
		if (start[i] != '\n')
			start[i] = ' ';

	return nowy;
}


//wyłapuje i kasuje makra oraz usuwa komentarze i stringi
//zwraca 0 gdy wszystko OK
int
preparsuj(kodplik_t *plik, listamakr_t *makra)
{
	long i;
	long poz_def = -1; //pozycja początku dyrektywy 'define'
	char cudz;         //typ cudzysłowy - do parsowania stringów
	stan_t stan = ST_KOD, stan_poprz = ST_KOD; //typy obiektów w których się znajdujemy podczas parsowania kodu

	//trochę lukru syntaktycznego żeby żywot programisty był słodszy:

	//wskaźnik do bierzącego znaku
	#define CC (plik->kod + i)

	//wskaźnik do nastepnego znaku
	#define CCN (plik->kod + i + 1)

	//kasuj bierzący znak
	#define KASUJCC() *(plik->kod + i) = ' '

	//kasuj bierzący i nastepny znak
	#define KASUJCC2() *(plik->kod + i) = *(plik->kod + i + 1) = ' '

	//zmień aktualny stan na A
	#define ZSTAN(A) stan_poprz = stan, stan = A

	//przywróć poprzedni stan a stan A zapisz jako poprzedni
	#define ZSTAN_R(A) stan = stan_poprz, stan_poprz = A

	//w pętli sprawdzamy cały plik znak po znaku
	for (i=0; i < plik->rozmiar; i++)
	{
		//sprawdzamy aktualny stan parsera
		switch (stan)
		{
			//kod programu
			case ST_KOD:
				switch (*CC)
				{
					case '#':
						KASUJCC();
						ZSTAN(ST_MAKRO);
					break;
					case '/':
						if (*CCN == '*')
						{
							ZSTAN(ST_KOMENTARZ_C);
							KASUJCC2();
							i++;
						}
						else if (*CCN == '/')
						{
							ZSTAN(ST_KOMENTARZ_CPP);
							KASUJCC2();
							i++;
						}
					break;
					case '\"':
					case '\'':
						ZSTAN(ST_STRING);
						cudz = *CC; //zapisujemy typ cudzysłowu w który wchodzimy

						*CC = 's'; //początek zawsze tym zastępujemy, żeby potem nie było "dziur" po skasowanych stringach
						           //wartośc wpisywanego znaku nie ma znaczenia
					break;
				}
			break;

			//makro preprocesora
			case ST_MAKRO:
				if (*CC == '/')
				{
					if (*CCN == '*')
					{
						ZSTAN(ST_KOMENTARZ_C);
						KASUJCC2();
						i++;
					}
					else if (*CCN == '/')
					{
						ZSTAN(ST_KOMENTARZ_CPP);
						KASUJCC2();
						//jeśli wcześniej wykryliśmy dyrektywę 'define'
						if (poz_def >= 0)
						{
							//dodajemy nazwę nowego makra
							char *nazwa_m = parsuj_nazwe_makra(CC-(i-poz_def), i-poz_def);
							listamakr_add(makra, nazwa_m);
							free(nazwa_m);
							poz_def = -1;
						}
						i++;
					}
				}
				else if (*CC == '\\' && *CCN == '\n')
				{
						KASUJCC();
						i+=2;
				}
				else if (*CC == '\n')
				{
					//jeśli wcześniej wykryliśmy dyrektywę 'define'
					if (poz_def >= 0)
					{
						//dodajemy nazwę nowego makra
						char *nazwa_m = parsuj_nazwe_makra(CC-(i-poz_def), i-poz_def);
						listamakr_add(makra, nazwa_m);
						free(nazwa_m);
						poz_def = -1;
					}
					ZSTAN(ST_KOD);
				}
				else if (*CC == 'd' && !strncmp(CC, "define", 6))
					poz_def = i += 6;
				//jesli nie jestesmy wewnątrz żadnego 'define' to od razu kasujemy treść
				else if (poz_def < 0)
					*CC = ' ';
			break;

			//wnętrze komentarza C
			case ST_KOMENTARZ_C:
				if (*CC == '*' && *CCN == '/')
				{
					ZSTAN_R(ST_KOMENTARZ_C);
					KASUJCC2();
					i++;
				}
				else if (*CC != '\n')
					KASUJCC();
			break;

			//wnętrze komentarza C++
			case ST_KOMENTARZ_CPP:
				if (*CC == '\n')
					ZSTAN(ST_KOD);
				else
					KASUJCC();
			break;

			//wnętrze stringa lub chara
			case ST_STRING:
				switch (*CC)
				{
					case '\\':
						KASUJCC();
						if (*CCN == '\"' || *CCN == '\'' || *CCN == '\\')
						{
							i++;
							KASUJCC();
						}
					break;
					case '\"':
						if (*CC == cudz)
							ZSTAN_R(ST_STRING);
						KASUJCC();
					break;
					case '\'':
						if (*CC == cudz)
							ZSTAN_R(ST_STRING);
						KASUJCC();
					break;
					default:
						KASUJCC();
				}
			break;
		}
	}

	#ifdef DEBUG
	//Blok testów

	//printf("Kod pliku' po parsowaniu:\n%s\n\n", plik->kod);
	listamakr_print(stdout, makra);
	#endif

	return 0;

	//sprzątamy
	#undef CC
	#undef KASUJCC
	#undef KASUJCC2
	#undef ZSTAN
	#undef ZSTAN_R
}

//parsuje i zwraca nazwę funkcji, zwraca NULL w przypadku niepowodzenia/gdy to nie jest funkcja
char *
to_funkcja(kodplik_t *plik, unsigned long znak)
{
	//musimy się cofać aż natrafimy na pierwszy nie-biały znak i musi nim być ')' - inaczej to nie funkcja!
	long i;
	long nawias = 0; //poziom zagłębienia nawiasów
	unsigned long poczatek, koniec;
	for (i=znak; i > 0; i--)
	{
		if (!isspace(plik->kod[i]))
		{
			if (plik->kod[i] == ')')
			{
				nawias--;
				break;
			}
			else
				return NULL; //bo to nie jest funkcja
		}
	}

	//jesli doszliśmy do poczatku pliku i na nic nie natrafilismy
	if (nawias == 0)
		return NULL;

	//szukamy nawiasku okragłego otwierającego funkcji
	for (i--; i > 0; i--)
	{
		switch (plik->kod[i])
		{
			case '(':
				nawias++;
			break;
			case ')':
				nawias--;
			break;
		}
		//sprawdzamy czy wyszliśmy ze wszystkich nawiasów
		if (!nawias)
			break;
	}

	//jesli doszliśmy do poczatku pliku i na nic nie natrafilismy
	if (nawias != 0)
		return NULL;

	//szukamy końca nazwy funkcji - przed nawiasem mogą być spacje!
	for ( ; i > 0; i--)
		if (!isspace(plik->kod[i]))
			break;

	//jesli doszliśmy do poczatku pliku i na nic nie natrafilismy
	if (i == 0)
		return NULL;

	//zapisujemy gdzie jest koniec nazwy
	koniec = i;

	//szukamy początku nazwy funkcji
	for ( i--; i > 0; i--)
		if (!isalpha(plik->kod[i]) && plik->kod[i] != '_')
			break;

	//zapisujemy gdzie jest początek nazwy
	poczatek = i+1;

	//tworzymy nowego stringa
	koniec -= poczatek;
	char *nowy = malloc((koniec+1) * sizeof (char));
	strncpy(nowy, &(plik->kod[poczatek]), koniec);
	nowy[koniec] = '\0';

	return nowy;
}

//wykrywa funkcje w pliku
listafunkcji_t *
wykryj_funkcje(kodplik_t *plik)
{
	unsigned long i;
	long klamry = 0;              //poziom zagłebienia w nawiasy klamrowe
	unsigned long linia = 1;     //numer linii w pliku
	char ignoruj_zamkniecie = 1;
	char *nazwafunkcji;
	listafunkcji_t *listafunkcji;
	funkcja_t *funkcja;

	//alokowanie pamieci
	if ((listafunkcji = listafunkcji_create()) == NULL)
		return NULL;

	//analizowanie kodu
	for (i=0; i < plik->rozmiar; i++)
	{
		switch (plik->kod[i])
		{
			case '{':
				if (klamry == 0)
				{
					//dopisujemy nową funkcję, na razie nie wiemy gdzie się kończy
					if ((nazwafunkcji = to_funkcja(plik, i-1)) != NULL)
					{
						if ((funkcja = funkcja_create(nazwafunkcji, D_FUNKCJA, i+1, 0, linia, 0)) == NULL)
						{
							free(nazwafunkcji);
							listafunkcji_free(listafunkcji);
							return NULL;
						}

						free(nazwafunkcji);
						listafunkcji_add(listafunkcji, funkcja);
						ignoruj_zamkniecie = 0;
					}
					else
						ignoruj_zamkniecie = 1; //gdy to nie funkcja trzeba zignorowac najbliższe wyjście z nawiasów
				}
				klamry++;
			break;
			case '}':
				klamry--;
				if (klamry == 0) //znaleźliśmy koniec znalezionej ostatnio funkcji
				{
					if (!ignoruj_zamkniecie)
					{
						listafunkcji->funkcje[listafunkcji->liczba-1]->znak_stop = i-1;
						listafunkcji->funkcje[listafunkcji->liczba-1]->linia_stop = linia;
					}
					else
						ignoruj_zamkniecie = 0;
				}
			break;
			case '\n': //nowa linia
				linia++;
			break;
			case '\0': //nieoczekiwany koniec pliku
				listafunkcji_free(listafunkcji);
				return NULL;
			break;
			default:
				;
		}
	}

	//jezeli liczba nawiasów się nie zgadza zasygnalizuj błąd
	if (klamry != 0)
	{
		listafunkcji_free(listafunkcji);
		return NULL;
	}

	return listafunkcji;
}



// sprawdza czy dana funkcja jest na podanej liście ignorowanych
char
czy_ignorowana(const ignfunkcje_t *ignorowane, const char *nazwafunkcji)
{
	if (ignorowane == NULL)
		return 0;

	unsigned int i;
	// jesli jest na liście to ją ignoruj
	for (i=0; i < ignorowane->liczba_funkcji; i++)
		if (!strcmp(nazwafunkcji, ignorowane->lista_funkcji[i]))
			return 1;

	//w przeciwnym razie
	return 0;
}

//sprawdza czy dane wywołanie funkcji jest makrem
typdrzewa_t
to_makro(const listamakr_t *makra, const char *nazwafunkcji)
{
	if (makra == NULL)
		return D_FUNKCJA_WYW;

	unsigned int i;
	for (i=0; i < makra->liczba; i++)
		if (!strcmp(nazwafunkcji, makra->nazwy[i]))
			return D_MAKRO;

	//w przeciwnym razie
	return D_FUNKCJA_WYW;
}

// parsuje i zwraca nazwę wywoływanej funkcji, zwraca NULL w przypadku
// niepowodzenia/gdy to nie jest wywołanie funkcji
char *
to_wywolanie_funkcji(kodplik_t *plik, const funkcja_t *funkcja, unsigned long znak)
{
	long i;
	long nawias = 0; //poziom zagłębienia się w nawiasy
	unsigned long poczatek, koniec;

	//szukamy końca nazwy wywołania funkcji - przed nawiasem mogą być spacje!
	for (i=znak; i >= funkcja->znak_start; i--)
		if (!isspace(plik->kod[i]))
			break;

	//jesli doszliśmy do poczatku wnetrza danej funkcji i na nic nie natrafilismy
	if (i < funkcja->znak_start)
		return NULL;

	//sprawdzamy czy to może byc nazwa funkcji
	if (!isalpha(plik->kod[i]) && plik->kod[i] != '_')
		//sprawdzamy czy to może być wywołanie poprzez wskaźnik na funkcję
		if (plik->kod[i] == ')')
		{
			//szukamy końca nazwy wywołania funkcji - przed nawiasem mogą być spacje!
			for (i--; i >= funkcja->znak_start; i--)
				if (!isspace(plik->kod[i]))
					break;

			//jesli doszliśmy do poczatku wnetrza danej funkcji i na nic nie natrafilismy
			if (i < funkcja->znak_start)
				return NULL;

			//sprawdzamy czy to może byc nazwa funkcji
			if (!isalpha(plik->kod[i]) && plik->kod[i] != '_')
				return NULL;
		}
		else
			return NULL;

	//zapisujemy gdzie jest koniec nazwy
	koniec = i+1;

	//szukamy początku nazwy wywoływanej funkcji
	for ( i--; i >= funkcja->znak_start; i--)
		if (!isalpha(plik->kod[i]) && plik->kod[i] != '_')
			break;

	//zapisujemy gdzie jest początek nazwy i jej długość
	poczatek = i+1;
	koniec -= poczatek;

	//kontrolujemy czy to na pewno funkcja
	if (!strncmp(&(plik->kod[poczatek]), "if", 2) ||
			!strncmp(&(plik->kod[poczatek]), "for", 3) ||
			!strncmp(&(plik->kod[poczatek]), "switch", 6) ||
			!strncmp(&(plik->kod[poczatek]), "sizeof", 6) ||
			!strncmp(&(plik->kod[poczatek]), "while", 5))
		return NULL;

	//jesli tak to ją zapisujemy
	char *nowy = malloc((koniec+1) * sizeof (char));
	strncpy(nowy, &(plik->kod[poczatek]), koniec);
	nowy[koniec] = '\0';

	return nowy;
}

//rekurencyjnie wykrywa wywołania funkcji w danej funkcji
//opcjonalnie uwzględnia ignorowane i informowanie o makrach
drzewo_t *
wykryj_wywolania(kodplik_t *plik, const funkcja_t *funkcja, const ignfunkcje_t *ignorowane, const listamakr_t *makra)
{
	drzewo_t *drzewo, *potomek;

	unsigned long linia = funkcja->linia_start;
	unsigned long i;
	long nawiasy = 0;     //zliczanie par nawiasów
	long zaglebienie = 0; //poziom zagłębienia w nawiasy

	char ignoruj_zamkniecie = 1;
	char *nazwafunkcji;
	listafunkcji_t *listafunkcji;
	funkcja_t *wywolanie;

	//alokowanie pamięci
	if ((drzewo = drzewo_create(funkcja->typ != D_FUNKCJA ? to_makro(makra, funkcja->nazwa) : D_FUNKCJA,
			funkcja->nazwa, funkcja->linia_start, funkcja->linia_stop)) == NULL)
		return NULL;

	if ((listafunkcji = listafunkcji_create()) == NULL)
	{
		drzewo_free(drzewo);
		return NULL;
	}

	//analizowanie kodu
	for (i=funkcja->znak_start; i <= funkcja->znak_stop; i++)
	{
		//sprawdzanie typu bierzącego znaku
		switch (plik->kod[i])
		{
			//wejście w nawias
			case '(':
				if (nawiasy == zaglebienie)
				{
					//dopisujemy nowe wywołanie funkcji, na razie nie wiemy gdzie się kończy
					if ((nazwafunkcji = to_wywolanie_funkcji(plik, funkcja, i-1)) != NULL)
					{
						//sprawdzamy czy nie jest ignorowana
						if (!czy_ignorowana(ignorowane, nazwafunkcji))
						{
							if ((wywolanie = funkcja_create(nazwafunkcji, D_FUNKCJA_WYW, i+1, 0, linia, 0)) == NULL)
							{
								free(nazwafunkcji);
								drzewo_free(drzewo);
								listafunkcji_free(listafunkcji);
								return NULL;
							}

							listafunkcji_add(listafunkcji, wywolanie);
							ignoruj_zamkniecie = 0;
						}
						else
							ignoruj_zamkniecie = 1;

						free(nazwafunkcji);
					}
					else
						zaglebienie++; //gdy to nie funkcja trzeba zignorowac najbliższe wyjście z nawiasów
				}
				nawiasy++;
			break;

			//wyjście z nawiasu
			case ')':
				nawiasy--;

				//znaleźliśmy koniec znalezionego ostatnio wywołania funkcji
				if (nawiasy == zaglebienie)
				{
					if (!ignoruj_zamkniecie)
					{
						listafunkcji->funkcje[listafunkcji->liczba-1]->znak_stop = i-1;
						listafunkcji->funkcje[listafunkcji->liczba-1]->linia_stop = linia;
					}
					else
						ignoruj_zamkniecie = 0;
				}

				//oznaczamy jeśli wyszliśmy z danego poziomu zagłębienia nawiasów
				if (nawiasy == zaglebienie-1)
					zaglebienie--;
			break;

			//nowa linia
			case '\n':
				linia++;
			break;
		}
	}

	//jezeli liczba nawiasów się nie zgadza zasygnalizuj błąd
	if (nawiasy != 0 || zaglebienie != 0)
	{
		drzewo_free(drzewo);
		listafunkcji_free(listafunkcji);
		return NULL;
	}

	//podpięcie każdego znalezionego wywołania
	for (i=0; i < listafunkcji->liczba; i++)
	{
		if ((potomek = wykryj_wywolania(plik, listafunkcji->funkcje[i], ignorowane, makra)) == NULL)
		{
			drzewo_free(drzewo);
			listafunkcji_free(listafunkcji);
			return NULL;
		}
		drzewo_add(drzewo, potomek);
	}

	listafunkcji_free(listafunkcji);
	return drzewo;
}


/******************************************************************************
							Interfejs modułu
*******************************************************************************/

drzewo_t *
generuj_drzewo(kodplik_t *plik, const ignfunkcje_t *ignorowane_funkcje, const char ignoruj_makra)
{
	//zmienne robocze
	listamakr_t *makra;
	drzewo_t *drzewo;
	listafunkcji_t *funkcje;

	//tworzymy listę makr
	if ((makra = listamakr_create()) == NULL)
		return NULL;

	//tworzymy drzewo do przechowywania danych
	if ((drzewo = drzewo_create(D_PLIK, plik->nazwa, 1, zlicz_linie(plik, 1, plik->rozmiar-1))) == NULL)
	{
		listamakr_free(makra);
		return NULL;
	}

	//przeprowadzamy wstepną obróbkę - wykrycie makr i wyczyszczenie kodu ze zbędnych rzeczy
	if (preparsuj(plik, makra))
	{
		listamakr_free(makra);
		drzewo_free(drzewo);
		return NULL;
	}

	//wykrywamy funkcje
	if ((funkcje = wykryj_funkcje(plik)) == NULL)
	{
		listamakr_free(makra);
		drzewo_free(drzewo);
		return NULL;
	}

	//parsujemy wnętrze każdej funkcji w poszukiwaniu wywołan innych funkcji
	unsigned int i;
	drzewo_t *poddrzewo;
	for (i=0; i < funkcje->liczba; i++)
	{
		if (ignoruj_makra)
			poddrzewo = wykryj_wywolania(plik, funkcje->funkcje[i], ignorowane_funkcje, makra);
		else
			poddrzewo = wykryj_wywolania(plik, funkcje->funkcje[i], ignorowane_funkcje, NULL);

		if (poddrzewo == NULL)
		{
			listamakr_free(makra);
			drzewo_free(drzewo);
			listafunkcji_free(funkcje);
			return NULL;
		}

		drzewo_add(drzewo, poddrzewo);
	}

	listamakr_free(makra);
	listafunkcji_free(funkcje);
	return drzewo;
}

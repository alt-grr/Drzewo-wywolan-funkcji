#include "struktury.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAX_FUNKCJA 256 //maksymalny rozmiar bufora nazw funkcji do czytania ich z pliku

/***************************
	Obsługa kodplik_t
****************************/

//wczytuje zawartośc pliku do parsowania
kodplik_t *
kodplik_load(const char *nazwa_pliku)
{
	FILE *plik;
	char *nazwa;
	long dlugosc_pliku;
	kodplik_t *dane;
	int i = 0;

	if ((plik = fopen(nazwa_pliku, "r")) == NULL)
		return NULL;

	//ustalamy rozmiar pliku
	fseek(plik, 0, SEEK_END);
	dlugosc_pliku = ftell(plik);
	fseek(plik, 0, SEEK_SET);

	//gdy plik pusty nic nie sparsujemy
	if (dlugosc_pliku < 1L)
	{
		fclose(plik);
		return NULL;
	}

	//alokacja pamięci na strukturę
	if ((dane = malloc(dlugosc_pliku * sizeof (kodplik_t))) != NULL)
	{
		//alokacja pamięci na treść pliku
		if ((dane->kod = malloc((dlugosc_pliku+1) * sizeof (char))) == NULL)
		{
			free(dane);
			fclose(plik);
			return NULL;
		}
		dane->rozmiar = dlugosc_pliku;

		//alokacja pamięci na nazwę pliku
		if ((nazwa = malloc((strlen(nazwa_pliku) + 1) * sizeof (char))) != NULL)
		{
			strcpy(nazwa, nazwa_pliku);
			dane->nazwa = nazwa;

		}
		else
		{
			free(dane->kod);
			free(dane);
			fclose(plik);
			return NULL;
		}
	}
	else
	{
		fclose(plik);
		return NULL;
	}

	//wczytanie pliku do pamięci
	for (i=0; i < dane->rozmiar; i++)
		dane->kod[i] = getc(plik);

	//terminator może być przydatny!
	dane->kod[i] = '\0';

	fclose(plik);
	return dane;
}

//destruktor
void
kodplik_free(kodplik_t *dane)
{
	if (dane->kod != NULL)
		free(dane->kod);
	if (dane->nazwa != NULL)
		free(dane->nazwa);
	free(dane);
}


/***************************
	Obsługa ignfunkcje_t
****************************/

//wczytuje listę ignorowanych funkcji
ignfunkcje_t *
ignfunkcje_load(const char *nazwa_pliku)
{
	FILE *plik;
	unsigned int liczba_funkcji;
	ignfunkcje_t *funkcje;

	if ((plik = fopen(nazwa_pliku, "r")) == NULL)
		return NULL;

	//ustalenie liczby funkcji do wczytania
	if (fscanf(plik, "%d", &liczba_funkcji) == EOF)
	{
		fclose(plik);
		return NULL;
	}

	//zaalokowanie pamieci na strukturę przechowującą listę funkcji
	if ((funkcje = malloc(sizeof (ignfunkcje_t))) == NULL)
	{
		fclose(plik);
		return NULL;
	}

	funkcje->liczba_funkcji = 0; //potrzebne do ewentualnego prawidłowego awaryjnego zwolnienia pamięci
	if ((funkcje->lista_funkcji = malloc(liczba_funkcji * sizeof (char*))) == NULL)
	{
		ignfunkcje_free(funkcje);
		fclose(plik);
		return NULL;
	}

	//po kolei wczytanie wszystkich funkcji do tablicy wskaźników

	char tmp[MAX_FUNKCJA]; //bufor na czytane nazwy
	for ( ; funkcje->liczba_funkcji < liczba_funkcji; funkcje->liczba_funkcji++)
	{
		if (fscanf(plik, "%s", &tmp) == EOF
				|| (funkcje->lista_funkcji[funkcje->liczba_funkcji] = malloc((strlen((char*)&tmp)+1) * sizeof (char))) == NULL)
		{
			fclose(plik);
			ignfunkcje_free(funkcje);
			return NULL;
		}

		strcpy(funkcje->lista_funkcji[funkcje->liczba_funkcji], (char*)&tmp);
	}

	fclose(plik);
	return funkcje;
}

//zwalnia pamięć zajętą przez listę funkcji
void
ignfunkcje_free(ignfunkcje_t *dane)
{
	unsigned int i;

	if (dane->lista_funkcji != NULL)
	{
		for (i=0; i < dane->liczba_funkcji; i++)
			if (dane->lista_funkcji[i] != NULL)
				free(dane->lista_funkcji[i]);

		free(dane->lista_funkcji);
	}

	free(dane);
}



/***************************
	Obsługa drzewo_t
****************************/

//utworzenie pojedynczego elementu drzewa
drzewo_t *
drzewo_create(const typdrzewa_t typ, const char *nazwa, const long poczatek, const long koniec)
{
	drzewo_t *drzewo;
	//alokujemy pamięć na strukturę drzewa i jej zawartość
	if ((drzewo = malloc(sizeof (drzewo_t))) == NULL)
		return NULL;

	if ((drzewo->nazwa = malloc((strlen(nazwa)+1) * sizeof (char))) == NULL)
	{
		free(drzewo);
		return NULL;
	}

	//wypełniamy strukturę danymi
	drzewo->typ = typ;
	strcpy(drzewo->nazwa, nazwa);
	drzewo->zakres[0] = poczatek;
	drzewo->zakres[1] = koniec;
	drzewo->liczba_potomkow = 0;
	drzewo->potomki = NULL;

	return drzewo;
}

//rekurencyjne zwolnienie pamięci zajętej przez podane drzewo
void
drzewo_free(drzewo_t *drzewo)
{
	if (drzewo->liczba_potomkow != 0)
	{
		long i;
		for (i=0; i < drzewo->liczba_potomkow; i++)
			if (drzewo->potomki[i] != NULL)
				drzewo_free(drzewo->potomki[i]);
	}

	#ifdef DEBUG
	//Test funkcji:
	printf("Zwalnianie %s\n", drzewo->nazwa);
	#endif

	free(drzewo->nazwa);
	free(drzewo->potomki);
	free(drzewo);
}


//podpięcie potomka do wybranego rodzica
drzewo_t *
drzewo_add(drzewo_t *rodzic, drzewo_t *potomek)
{
	//alokacja miejsca na pierwszego potomka
	if (rodzic->liczba_potomkow == 0)
	{
		if ((rodzic->potomki = malloc(sizeof (drzewo_t*))) == NULL)
			return NULL;
	}
	//robimy miejsce na dopisanie nowego potomka
	else if (rodzic->liczba_potomkow == UINT_MAX
				|| (rodzic->potomki = realloc(rodzic->potomki, (rodzic->liczba_potomkow+1) * sizeof (drzewo_t*))) == NULL)
		return NULL;

	rodzic->potomki[rodzic->liczba_potomkow++] = (drzewo_t*)potomek; //bez rzutowania kompilator kręci nosem

	//sprawdzamy czy to wywołanie rekurencyjne - jesli tak to modyfikujemy nazwę
	if (!strcmp(rodzic->nazwa, potomek->nazwa))
	{
		unsigned int dl = strlen(potomek->nazwa);

		                                              // 14 znaków to długość stringa " (rekurencja)"
		if ((potomek->nazwa = realloc(potomek->nazwa, (dl + 14)*sizeof (char))) == NULL)
			return NULL;
		strcpy(&potomek->nazwa[dl], " (rekurencja)");
	}

	return rodzic->potomki[rodzic->liczba_potomkow-1];
}

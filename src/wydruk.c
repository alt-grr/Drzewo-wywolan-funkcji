#include "wydruk.h"
#include <stdlib.h>
#include <string.h>

/***************************
	Prywatne funkcje modułu
****************************/

//zwraca tekstowy opis typu węzła drzewa
char *
typdrzewatostr(typdrzewa_t typ)
{
	switch (typ)
	{
		case D_PLIK:
			return "Plik";
		break;
		case D_FUNKCJA:
			return "Funkcja";
		break;
		case D_FUNKCJA_WYW:
			return "Wywołuje";
		break;
		case D_MAKRO:
			return "Makro";
		break;
	}
}

//zwraca string złożony z "liczba" znaków "znak"
char *
strofchar(const char znak, const unsigned int liczba)
{
	char *tmp = malloc((liczba+1) * sizeof (char));
	if (tmp == NULL)
		return NULL;

	memset(tmp, znak, liczba);
	tmp[liczba] = '\0';

	return tmp;
}

//rekurencyjnie drukuje drzewo
void
drukuj_drzewo_r(FILE *wyjscie, const drzewo_t *drzewo, const char wypisuj_numery, unsigned int glebokosc)
{
	static char zakres[42]; /* LONG_MAX zajmuje po konwersji na tekst maksymalnie 19 znaków (2^63 -1)
	                          * deklaracja jako static oszczędza sporo pamięci przy rozbudowanych drzewach
	                          */
	char *dwukropek = (drzewo->liczba_potomkow == 0 ? "" : ":");
	char *wciecie = strofchar('\t', glebokosc);

	//ustalamy numery linii jeśli trzeba
	if (wypisuj_numery)
	{
		if (drzewo->zakres[0] == drzewo->zakres[1])
			sprintf((char*)&zakres, "[%ld]", drzewo->zakres[0]);
		else
			sprintf((char*)&zakres, "[%ld-%ld]", drzewo->zakres[0], drzewo->zakres[1]);

		fprintf(wyjscie, "%s %s %s%s\n", typdrzewatostr(drzewo->typ), drzewo->nazwa, &zakres, dwukropek);
	}
	else
	//wypisujemy
		fprintf(wyjscie, "%s %s%s\n", typdrzewatostr(drzewo->typ), drzewo->nazwa, dwukropek);

	//wypisujemy wszystkie niepuste potomki
	unsigned int i;
	for (i=0; i < drzewo->liczba_potomkow; i++)
	{
		if (drzewo->potomki[i] != NULL)
		{
			fprintf(wyjscie, "%s", wciecie);
			drukuj_drzewo_r(wyjscie, drzewo->potomki[i], wypisuj_numery, glebokosc+1);
		}
	}
	free(wciecie);
}

/***************************
	Funkcje modułu
****************************/

//drukuje drzewo
void
drukuj_drzewo(FILE *wyjscie, const drzewo_t *drzewo, const char wypisuj_numery)
{
	drukuj_drzewo_r(wyjscie, drzewo, wypisuj_numery, 1);
}

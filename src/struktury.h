#ifndef _STRUKTURY_H_
#define _STRUKTURY_H_

#include <stdlib.h>

/*****************************************************
	struktura przechowująca plik do parsowania
******************************************************/

typedef struct {
	char *nazwa;  //nazwa pliku
	long rozmiar; //rozmiar w charach, bez końcowego terminatora
	char *kod;    //treść pliku
} kodplik_t;

kodplik_t *
kodplik_load(const char *nazwa_pliku);

void
kodplik_free(kodplik_t *dane);

/*****************************************************
	struktura przechowująca ignorowane funkcje
******************************************************/

typedef struct {
	unsigned int liczba_funkcji;
	char **lista_funkcji;
} ignfunkcje_t;

ignfunkcje_t *
ignfunkcje_load(const char *nazwa_pliku);

void
ignfunkcje_free(ignfunkcje_t *dane);

/*****************************************************
	struktura do tworzenia drzewa wywołań
******************************************************/

//typ węzła drzewa
typedef enum {
	D_PLIK,       //plik
	D_FUNKCJA,    //funkcja i jej treść
	D_FUNKCJA_WYW,//wywołanie funkcji
	D_MAKRO       //makro
} typdrzewa_t;

typedef struct drzewo_t {
	typdrzewa_t typ;
	char *nazwa;
	long zakres[2]; //wektor z numerami znaków: początkowego i końcowego
	unsigned int liczba_potomkow;
	struct drzewo_t **potomki; //tablica wskaźników do potomków
} drzewo_t;

drzewo_t *
drzewo_create(const typdrzewa_t typ, const char *nazwa, const long poczatek, const long koniec);

void
drzewo_free(drzewo_t *drzewo);

drzewo_t *
drzewo_add(drzewo_t *rodzic, drzewo_t *potomek);

#endif

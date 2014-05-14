#ifndef _PARSER_H_
#define _PARSER_H_

#include "struktury.h"

drzewo_t *
generuj_drzewo(kodplik_t *plik, const ignfunkcje_t *ignorowane_funkcje, const char ignoruj_makra);

#endif

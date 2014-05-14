#include <stdio.h>

#include "struktury.h"
#include "parser.h"
#include "wydruk.h"

#define IGNOROWANE_FUNKCJE_STD "ignoruj.txt"

int
main(int argc, char **argv)
{
	/* Opcje konfiguracyjne */
	char ignoruj_makra = 0;
	char ignoruj_funkcje = 0;
	char wypisuj_numery = 0;
	char *ignorowane_funkcje_plik = NULL;

	/* zmienne robocze */
	ignfunkcje_t *ignorowane_funkcje = NULL;
	kodplik_t *tresc_pliku;
	drzewo_t *drzewo;

	//odpalenie bez parametrów - pokaż manual
	if (argc < 2)
	{
		printf(
			"Parametry wywołania:\n"
			"====================\n"
			"-m\t\tinformuj o makrodefinicjach\n"
			"-n\t\twypisuj numery wierszy\n"
			"-i\t\tignoruj domyślny zestaw funkcji z pliku " IGNOROWANE_FUNKCJE_STD "\n"
			"-i:lista\tignoruj zestaw funkcji z pliku \"lista\"\n"
			"inne\t\tnazwy plików do przetworzenia\n");
		return 0;
	}

	int i;

	/* Pierwszy przebieg po liście parametrów - sprawdzamy czy są podane jakieś opcje */
	for (i=1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] == 'm')
				ignoruj_makra = 1;
			else if (argv[i][1] == 'n')
				wypisuj_numery = 1;
			else if (argv[i][1] == 'i')
			{
				ignoruj_funkcje = 1;

				if (argv[i][2] == ':' && argv[i][3] != '\0')
					ignorowane_funkcje_plik = &(argv[i][3]);
			}
			else
			{
				fprintf(stderr, "%s: Błąd - nieznany parametr %s\n", argv[0], argv[i]);
				return 1;
			}
		}
	}

	/* wczytujemy listę ignorowanych funkcji jeśli ma jakieś ignorować */
	if (ignoruj_funkcje)
	{
		//czytamy z podanego przez użytkownika pliku
		if (ignorowane_funkcje_plik != NULL)
		{
			if ((ignorowane_funkcje = ignfunkcje_load(ignorowane_funkcje_plik)) == NULL)
			{
				fprintf(stderr, "%s: Błąd podczas próby wczytania pliku ignorowanych funkcji %s\n",
						argv[0], ignorowane_funkcje_plik);
				return 2;
			}

		}
		//albo z pliku domyślnego
		else if ((ignorowane_funkcje = ignfunkcje_load(IGNOROWANE_FUNKCJE_STD)) == NULL)
		{
			fprintf(stderr, "%s: Błąd podczas próby wczytania domyślnego pliku ignorowanych funkcji "
					IGNOROWANE_FUNKCJE_STD"\n", argv[0]);
			return 2;
		}

		#ifdef DEBUG
		//test czy wczytało je poprawnie
		for (i=0; i < ignorowane_funkcje->liczba_funkcji; i++)
			printf("Ignorowana funkcja nr %d: %s\n", i, ignorowane_funkcje->lista_funkcji[i]);
		#endif
	}

	/* wczytujemy po kolei wszystkie pliki, generujemy ich drzewa i je drukujemy */
	for (i=1; i < argc; i++)
	{
		//z minusem na początku są parametry a nie pliki
		if (argv[i][0] == '-')
			continue;

		//wczytujemy treść pliku
		if ((tresc_pliku = kodplik_load(argv[i])) == NULL)
		{
			fprintf(stderr, "%s: Błąd podczas próby wczytania pliku %s\n", argv[0], argv[i]);
			if (ignoruj_funkcje)
				ignfunkcje_free(ignorowane_funkcje);
			return 3;
		}

		//generujemy jego drzewo wywołań i je drukujemy
		if ((drzewo = generuj_drzewo(tresc_pliku, ignorowane_funkcje, ignoruj_makra)) == NULL)
		{
			fprintf(stderr, "%s: Błąd podczas próby wygenerowania drzewa wywołań funkcji z pliku %s\n",
					argv[0], argv[i]);
			if (ignoruj_funkcje)
				ignfunkcje_free(ignorowane_funkcje);
			kodplik_free(tresc_pliku);
			return 4;
		}

		drukuj_drzewo(stdout, drzewo, wypisuj_numery);
		printf("\n");

		//sprzątamy przed nastepną iteracją
		kodplik_free(tresc_pliku);
		drzewo_free(drzewo);
	}

	if (ignoruj_funkcje)
		ignfunkcje_free(ignorowane_funkcje);

	return 0;
}

#define _XOPEN_SOURCE
#include <langinfo.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ASIZEOF(_a) (sizeof(_a) / sizeof(_a[0]))

#define FLAG_A	(1<<0)
#define FLAG_C	(1<<1)
#define FLAG_K	(1<<2)
#define FLAG_M	(1<<3)

static char *categories[] = {
	[LC_ALL] = "LC_ALL",
	[LC_COLLATE] = "LC_COLLATE",
	[LC_CTYPE] = "LC_CTYPE",
	[LC_MESSAGES] = "LC_MESSAGES",
	[LC_MONETARY] = "LC_MONETARY",
	[LC_NUMERIC] = "LC_NUMERIC",
	[LC_TIME] = "LC_TIME",
};

#define OFF(_m) (offsetof(struct lconv, _m)), {0}, 0

static struct {
	char *name;
	int cat;
	enum { NL, LCS, LCI, LCA } source;
	size_t noff;
	nl_item items[12];
	int print;
} keywords[] = {
	{ "codeset",	LC_CTYPE,	NL, 1, { CODESET }, 0 },
	{ "abday",	LC_TIME,	NL, 7, { ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4,
					ABDAY_5, ABDAY_6, ABDAY_7 }, 0 },
	{ "day",	LC_TIME,	NL, 7, { DAY_1, DAY_2, DAY_3, DAY_4, DAY_5,
					DAY_6, DAY_7 }, 0 },
	{ "abmon",	LC_TIME,	NL, 12, { ABMON_1, ABMON_2, ABMON_3,
					ABMON_4, ABMON_5, ABMON_6, ABMON_7,
					ABMON_8, ABMON_9, ABMON_10, ABMON_11,
					ABMON_12 }, 0 },
	{ "mon", 	LC_TIME, 	NL, 12, { MON_1, MON_2, MON_3, MON_4, MON_5,
					MON_6, MON_7, MON_8, MON_9, MON_10,
					MON_11, MON_12 }, 0 },
	{ "am_pm", 	LC_TIME,	NL, 2, { AM_STR, PM_STR }, 0 },
	{ "d_t_fmt",	LC_TIME,	NL, 1, { D_T_FMT }, 0 },
	{ "d_fmt",	LC_TIME,	NL, 1, { D_FMT }, 0 },
	{ "t_fmt",	LC_TIME,	NL, 1, { T_FMT }, 0 },
	{ "t_fmt_ampm",	LC_TIME,	NL, 1, { T_FMT_AMPM }, 0 },
	{ "era",	LC_TIME,	NL, 1, { ERA }, 0 },
	{ "era_d_fmt",	LC_TIME,	NL, 1, { ERA_D_FMT }, 0 },
	{ "era_d_t_fmt",LC_TIME,	NL, 1, { ERA_D_T_FMT }, 0 },
	{ "era_t_fmt",	LC_TIME,	NL, 1, { ERA_T_FMT }, 0 },
	{ "alt_digits",	LC_TIME,	NL, 1, { ALT_DIGITS }, 0 },
	{ "yesexp",	LC_MESSAGES,	NL, 1, { YESEXPR }, 0 },
	{ "noexpr",	LC_MESSAGES,	NL, 1, { NOEXPR }, 0 },
	{ "crncystr",	LC_MONETARY,	NL, 1, { CRNCYSTR }, 0 },
	{ "decimal_point",	LC_NUMERIC,	NL, 1, { RADIXCHAR }, 0 },
	{ "thousands_sep",	LC_NUMERIC,	NL, 1, { THOUSEP }, 0 },
	{ "int_curr_symbol",	LC_MONETARY,	LCS, OFF(int_curr_symbol) },
	{ "currency_symbol",	LC_MONETARY,	LCS, OFF(currency_symbol) },
	{ "mon_decimal_point",	LC_MONETARY,	LCS, OFF(mon_decimal_point) },
	{ "mon_thousands_sep",	LC_MONETARY,	LCS, OFF(mon_thousands_sep) },
	{ "mon_grouping",	LC_MONETARY,	LCA, OFF(mon_grouping) },
	{ "positive_sign",	LC_MONETARY,	LCS, OFF(positive_sign) },
	{ "negative_sign",	LC_MONETARY,	LCS, OFF(negative_sign) },
	{ "int_frac_digits",	LC_MONETARY,	LCI, OFF(int_frac_digits) },
	{ "frac_digits",	LC_MONETARY,	LCI, OFF(frac_digits) },
	{ "p_cs_precedes",	LC_MONETARY,	LCI, OFF(p_cs_precedes) },
	{ "p_sep_by_space",	LC_MONETARY,	LCI, OFF(p_sep_by_space) },
	{ "n_cs_precedes",	LC_MONETARY,	LCI, OFF(n_cs_precedes) },
	{ "n_sep_by_space",	LC_MONETARY,	LCI, OFF(n_sep_by_space) },
	{ "p_sign_posn",	LC_MONETARY,	LCI, OFF(p_sign_posn) },
	{ "n_sign_posn",	LC_MONETARY,	LCI, OFF(n_sign_posn) },
	{ "int_p_cs_precedes",	LC_MONETARY,	LCI, OFF(int_p_cs_precedes) },
	{ "int_n_cs_precedes",	LC_MONETARY,	LCI, OFF(int_n_cs_precedes) },
	{ "int_p_sep_by_space",	LC_MONETARY,	LCI, OFF(int_p_sep_by_space) },
	{ "int_n_sep_by_space",	LC_MONETARY,	LCI, OFF(int_n_sep_by_space) },
	{ "int_p_sign_posn",	LC_MONETARY,	LCI, OFF(int_p_sign_posn) },
	{ "int_n_sign_posn",	LC_MONETARY,	LCI, OFF(int_n_sign_posn) },
	{ "grouping",		LC_NUMERIC,	LCA, OFF(grouping) },
};

void printkw(size_t n, int flags)
{
	char *value = NULL;
	static struct lconv *lconv = NULL;
	if (lconv == NULL) {
		lconv = localeconv();
	}

	if (flags & FLAG_K) {
		printf("%s=", keywords[n].name);
	}

	if (keywords[n].source != NL) {
		value = ((char*)lconv) + keywords[n].noff;

		if (keywords[n].source == LCI) {
			printf("%d\n", *value);
			return;
		}

		if (flags & FLAG_K) {
			putchar('"');
		}

		/* value contains the address of a pointer to a string,
		 * not the address of a string itself, so dereference once */
		value = *((char**)value);

		if (keywords[n].source == LCS) {
			printf("%s", value);
		} else {
			/* LCA */
			while (*value) {
				printf("%hhd", *value);
				value++;
				if (*value) {
					putchar(';');
				}
			}
		}

		if (flags & FLAG_K) {
			putchar('"');
		}
		putchar('\n');
		return;
	}

	if (flags & FLAG_K) {
		putchar('"');
	}

	for (size_t i = 0; i < keywords[n].noff; i++) {
		value = nl_langinfo(keywords[n].items[i]);
		printf("%s", value);
		if (i < keywords[n].noff - 1) {
			putchar(';');
		}
	}

	if (flags & FLAG_K) {
		putchar('"');
	}
	putchar('\n');
}

int main(int argc, char *argv[])
{
	int flags = 0;
	int c;

	while ((c = getopt(argc, argv, "ackm")) != -1) {
		switch (c) {
		case 'a':
			flags |= FLAG_A;
			break;

		case 'c':
			flags |= FLAG_C;
			break;

		case 'k':
			flags |= FLAG_K;
			break;

		case 'm':
			flags |= FLAG_M;
			break;

		default:
			return 1;
		}
	}

	setlocale(LC_ALL, "");

	if (flags == FLAG_A) {
		if (argc >= optind) {
			fprintf(stderr, "no args to -a\n");
			return 1;
		}
		/* write about all public locales */
		/*
		for (size_t i = 0; i < number_of_public_locales; i++) {
			printf("%s\n", public_locale[i]);
		}
		*/
		return 0;
	}

	if (flags == FLAG_M) {
		if (argc >= optind) {
			fprintf(stderr, "no args to -m\n");
		}
		/* write available charmaps */
		/*
		for (size_t i = 0; i < number_of_available_charmaps; i++) {
			printf("%s\n", available_charmaps[i]);
		}
		*/
		return 0;
	}

	if ((flags & FLAG_A) || (flags & FLAG_M)) {
		fprintf(stderr, "can't combine -a or -m with other flags\n");
		return 1;
	}

	char *lang = getenv("LANG");
	printf("LANG=%s\n", lang ? lang : "");

	if (optind >= argc) {
		char *lc_all = getenv("LC_ALL");

		for (size_t i = 0; i < ASIZEOF(categories); i++) {
			if (i == LC_ALL || categories[i] == NULL) {
				continue;
			}

			char *val = getenv(categories[i]);
			if (val) {
				printf("%s=%s\n", categories[i], val);
			} else {
				printf("%s=\"%s\"\n", categories[i], lc_all ? lc_all : lang);
			}
		}

		printf("LC_ALL=%s\n", lc_all ? lc_all : "");
		return 0;
	}


	do {
		int found = 0;

		for (size_t i = 0; i < ASIZEOF(categories); i++) {
			if (i == LC_ALL || categories[i] == NULL) {
				continue;
			}

			if (!strcmp(argv[optind], categories[i])) {
				found = 1;
				for (size_t j = 0; j < ASIZEOF(keywords); j++) {
					if (keywords[j].cat == (int)i) {
						keywords[j].print = 1;
					}
				}
				break;
			}
		}

		if (found) {
			continue;
		}

		for (size_t i = 0; i < ASIZEOF(keywords); i++) {
			if (!strcmp(argv[optind], keywords[i].name)) {
				found = 1;
				keywords[i].print = 1;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "unknown keyword '%s'\n", argv[optind]);
		}
	} while (argv[++optind]);

	for (size_t i = 0; i < ASIZEOF(categories); i++) {
		if (i == LC_ALL || categories[i] == NULL) {
			continue;
		}

		int catprinted = 0;
		for (size_t j = 0; j < ASIZEOF(keywords); j++) {
			if (keywords[j].cat == (int)i && keywords[j].print == 1) {
				if ((flags & FLAG_C) && catprinted == 0) {
					printf("%s\n", categories[i]);
					catprinted = 1;
				}
				printkw(j, flags);
			}
		}
	}

	return 0;
}

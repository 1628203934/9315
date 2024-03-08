#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "lib/stringinfo.h"
#include "access/hash.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <regex.h>

PG_MODULE_MAGIC;

// PersonName definition
typedef struct PersonName
{
    int name_length;
    char name[1];
} PersonName;

bool validName(char *str){
    char *regex_pattern = "^[A-Z][A-Za-z'-]+([ ][A-Z][A-Za-z'-]+)*,[ ]?[A-Z][A-Za-z'-]+([ ][A-Z][A-Za-z'-]+)*$";
    regex_t regex;
    int flag;
    bool result = false;
    flag = regcomp(&regex, regex_pattern, REG_EXTENDED | REG_NOSUB);
    if (flag == 0) {
        if (!regexec(&regex, str, 0, NULL, 0)){
            result = true;
        }
    }
    regfree(&regex);
    return result;
}

// Function definition
PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    // to check whether input is valid
    if (!validName(str))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                               "PersonName", str)));
    char *given = strchr(str, ',');
    int family_len = given - str;
    if (*(given + 1) == ' ') given += 2;
    else given++;
    int length = family_len + strlen(given) + 2;
    PersonName *result = (PersonName *) palloc(VARHDRSZ + length);
    SET_VARSIZE(result, VARHDRSZ + length);
    str[family_len] = '\0';
    snprintf(result->name, length , "%s,%s", str, given);
    str[family_len] = ',';
    result->name_length = length - 1;
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    PG_RETURN_CSTRING(pname->name);
}

static int
pname_cmp_internal(PersonName *a, PersonName *b)
{
    char *comma_a = strchr(a->name, ',');
    char *comma_b = strchr(b->name, ',');
    int len_a = comma_a - a->name; // family name length
    int len_b = comma_b - b->name;
    int family_cmp = strncmp(a->name, b->name, len_a < len_b ? len_a : len_b); // compare family names
    if (family_cmp == 0) {
        if (len_a == len_b) return strcmp(comma_a + 1, comma_b + 1); // family names are equal, compare given names;
        else return len_a > len_b ? 1 : -1;
    } else return family_cmp < 0 ? -1 : 1;
}

PG_FUNCTION_INFO_V1(pname_lt);

Datum
pname_lt(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(pname_le);

Datum
pname_le(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(pname_eq);

Datum
pname_eq(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(pname_ne);

Datum
pname_ne(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) != 0);
}

PG_FUNCTION_INFO_V1(pname_ge);

Datum
pname_ge(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(pname_gt);

Datum
pname_gt(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(pname_cmp);

Datum
pname_cmp(PG_FUNCTION_ARGS)
{
    PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(pname_cmp_internal(a, b));
}

PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    char *comma = strchr(pname->name, ',');
    int family_length = comma - pname->name;
    text *family = (text *) palloc(VARHDRSZ + family_length);
    SET_VARSIZE(family, VARHDRSZ + family_length);
    memcpy(VARDATA(family), pname->name, family_length);
    PG_RETURN_TEXT_P(family);
}

PG_FUNCTION_INFO_V1(given);

Datum
given(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    char *comma = strchr(pname->name, ',');
    text *given = cstring_to_text(comma + 1);
    PG_RETURN_TEXT_P(given);
}

PG_FUNCTION_INFO_V1(show);

Datum
show(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    char *full_name = pstrdup(pname->name);
    char *family = strtok(full_name, ",");
    char *given = strtok(NULL, ",");
    char *first_given = strtok(given, " "); // extract the first part of given name splitted by space
    int str_len = strlen(family) + strlen(first_given) + 2;
    char *formatted_name = palloc(str_len);
    snprintf(formatted_name, str_len, "%s %s", first_given, family);
    text *result = cstring_to_text(formatted_name);
    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(pname_hash);

Datum
pname_hash(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    int hash_code = DatumGetUInt32(hash_any((unsigned char *) pname->name, pname->name_length));
    PG_RETURN_INT32(hash_code);
}

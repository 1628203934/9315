#include "postgres.h"
#include "fmgr.h"
#include "access/hash.h"
#include <string.h>
#include <stdbool.h>
#include <regex.h>

PG_MODULE_MAGIC;

typedef struct Pname
{
    int length;
    char name[];
} PersonName;

// use re to formalize string
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

PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    if (!validName(str))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                               "PersonName", str)));
    int length = strlen(str) + 1; // "1" for null terminator
    PersonName *result = (PersonName *) palloc(VARHDRSZ + length);
    SET_VARSIZE(result, VARHDRSZ + length);
    snprintf(result->name, length , "%s", str);
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    char *given = strchr(pname->name, ',');
    int family_len = given - pname->name;
    if (*(given + 1) == ' ') given += 2; // trim the space
    else given++;
    pname->name[family_len] = '\0';
    char *output = psprintf("%s,%s", pname->name, given);
    pname->name[family_len] = ',';
    PG_RETURN_CSTRING(output);
}

static int pname_cmp_internal(PersonName * a, PersonName * b)
{
    int a_comma = 0 , b_comma = 0;  // comma pos
    for (a_comma = 0; a_comma < strlen(a->name); a_comma++) {
        if (a->name[a_comma] == ',') {
            break;
        }
    }
    for (b_comma = 0; b_comma < strlen(b->name); b_comma++) {
        if (b->name[b_comma] == ',') {
            break;
        }
    }
    char *a_given = strchr(a->name, ',');
    if (*(a_given + 1) == ' ') a_given += 2;
    else a_given++;
    char *b_given = strchr(b->name, ',');
    if (*(b_given + 1) == ' ') b_given += 2;
    else b_given++;
    a->name[a_comma] = '\0';
    b->name[b_comma] = '\0';
    int family_cmp = strcmp(a->name, b->name); // compare family name
    a->name[a_comma] = ',';
    b->name[b_comma] = ',';
    if (family_cmp == 0) return strcmp(a_given, b_given);
    else return family_cmp;
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
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);
    PG_RETURN_INT32(pname_cmp_internal(a, b));
}

PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    int comma;
    for (comma = 0; comma < strlen(pname->name); comma++) {
        if (pname->name[comma] == ',') {
            break;
        }
    }
    pname->name[comma] = '\0';
    char *family = psprintf("%s", pname->name);
    pname->name[comma] = ',';
    int family_len = strlen(family);
    text *result = (text *) palloc(VARHDRSZ + family_len);
    SET_VARSIZE(result, VARHDRSZ + family_len);
    memcpy(VARDATA(result), family, family_len);
    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(given);

Datum
given(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    char *given = strrchr(pname->name, ',');
    if (*(given + 1) == ' ') given += 2;
    else given++;
    char *name = psprintf("%s", given);
    int given_len = strlen(name);
    text *result = (text *) palloc(VARHDRSZ + given_len);
    SET_VARSIZE(result, VARHDRSZ + given_len);
    memcpy(VARDATA(result), name, given_len);
    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(show);

Datum
show(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    char *name;
    char *given = strchr(pname->name, ',');
    int family_len = given - pname->name;
    if (*(given + 1) == ' ') given += 2;
    else given++;
    char *rest_given = strchr(given, ' ');
    pname->name[family_len] = '\0';
    // only one given name
    if (!rest_given) {
        name = psprintf("%s %s", given, pname->name);
    }
    else {
        int first_given_len = rest_given - given;
        *(given + first_given_len) = '\0';
        name = psprintf("%s %s", given, pname->name);
        *(given + first_given_len) = ' ';
    }
    pname->name[family_len] = ',';
    int name_length = strlen(name);
    text *result = (text *) palloc(VARHDRSZ + name_length);
    SET_VARSIZE(result, VARHDRSZ + name_length);
    memcpy(VARDATA(result), name, name_length);
    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(pname_hash);

Datum
pname_hash(PG_FUNCTION_ARGS)
{
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);
    int comma;
    for (comma = 0; comma < strlen(pname->name); comma++) {
        if (pname->name[comma] == ',') {
            break;
        }
    }
    char *given = strrchr(pname->name, ',');
    // overwriting space with given name
    if (*(given + 1) == ' ') {
        given += 2;
        comma++;
        while(*given != '\0') {
            pname->name[comma] = *given;
            given++;
            comma++;
        }
        pname->name[comma] = *given;
    }
    int hash_code = DatumGetUInt32(hash_any((unsigned char *) pname->name, strlen(pname->name)));
    PG_RETURN_INT32(hash_code);
}

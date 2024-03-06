#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

PG_MODULE_MAGIC;

// PersonName definition
typedef struct Pname
{
    text *family;
    text *given;
} Pname;

bool is_valid_name(char* name) {
    char *temp = strdup(name);
    size_t len = strlen(name);
    if (name[len - 1] == ' ' || len <= 1 || !isupper(name[0])) return false;
    for (int i = 1; i < len; i++) {
        if (!(isalpha(name[i]) || name[i] == '-' || name[i] == '\'' || name[i] == ' ') || (name[i] == ' ' && name[i + 1] == ' ')) return false;
    }
    temp = strtok(temp, " ");
    while (temp != NULL) {
        if (strlen(temp) < 2) {
            return false;
        }
        temp = strtok(NULL, " ");
    }
    return true;
}

// Function definition
PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
    text *input_text = PG_GETARG_TEXT_PP(0);
    char *str = text_to_cstring(input_text);
    char *comma = strchr(str, ',');
    // check validity of person name: 1. no comma 2. multiple commas 3. no family name 4. no given name 5. too many spaces after comma
    if (!comma || strchr(comma + 1, ',') || comma == str || *(comma + 1) == '\0' || *(comma + 2) == ' ')
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                errmsg("Incorrect name")));
    // separate person name into family and given names
    char *family = strtok(str, ",");
    char *given = strtok(NULL, ",");
    if (*(comma + 1) == ' ') given = comma + 2;
    if (given == NULL)
        ereport(ERROR,
           (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                   errmsg("Incorrect name")));
    // check validity of family and given names respectively
    if (!(is_valid_name(family) && is_valid_name(given)))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("Incorrect name")));
    // store two names using self-defined structure Pname
    Pname *input = (Pname *) palloc(sizeof(Pname));
    input->family = cstring_to_text(family);
    input->given = cstring_to_text(given);
    PG_RETURN_POINTER(input);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    char *output;
    char *family = text_to_cstring(pname->family);
    char *given = text_to_cstring(pname->given);

    StringInfoData str;
    initStringInfo(&str);
    appendStringInfo(&str, "%s, %s", family, given);
    text *output = cstring_to_text(str.data);

    pfree(str.data);
    pfree(family);
    pfree(given);
    PG_RETURN_TEXT_P(output);
}

int name_is_equal (char *a, char *b) {
    for (int i = 0; i < (strlen(a) > strlen(b) ? strlen(b) : strlen(a)); i++) {
        if (a[i] > b[i]) {
            return 1;
        } else if (a[i] < b[i]){
            return -1;
        }
    }
    if (strlen(a) == strlen(b)) {
        return 0;
    } else if (strlen(a) > strlen(b)) {
        return 1;
    } else {
        return -1;
    }
}

static int
pname_cmp_internal(Pname *a, Pname *b)
{
    char *f_a = text_to_cstring(a->family);
    char *g_a = text_to_cstring(a->given);
    char *f_b = text_to_cstring(b->family);
    char *g_b = text_to_cstring(b->given);

    if (name_is_equal(f_a, f_b) == 1) return 1;
    else if (name_is_equal(f_a, f_b) == -1) return -1;
    else {
        if (name_is_equal(g_a, g_b) == 1) return 1;
        else if (name_is_equal(g_a, g_b) == -1) return -1;
        else return 0;
    }
    return 0;
}

PG_FUNCTION_INFO_V1(pname_lt);

Datum
pname_lt(PG_FUNCTION_ARGS)
{
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(pname_le);

Datum
pname_le(PG_FUNCTION_ARGS)
{
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(pname_eq);

Datum
pname_eq(PG_FUNCTION_ARGS)
{
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(pname_ne);

Datum
pname_ne(PG_FUNCTION_ARGS)
{
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) != 0);
}

PG_FUNCTION_INFO_V1(pname_ge);

Datum
pname_ge(PG_FUNCTION_ARGS)
{
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(pname_gt);

Datum
pname_gt(PG_FUNCTION_ARGS)
{
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(pname_family);

Datum
pname_family(PG_FUNCTION_ARGS)
{
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    PG_RETURN_TEXT_P(pname->family);
}

PG_FUNCTION_INFO_V1(pname_given);

Datum
pname_given(PG_FUNCTION_ARGS)
{
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    PG_RETURN_TEXT_P(pname->given);
}

PG_FUNCTION_INFO_V1(pname_full);

Datum
pname_full(PG_FUNCTION_ARGS)
{
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    char* family = text_to_cstring(pname->family);
    char* given = text_to_cstring(pname->given);
    char* first_given = strtok(given, " ");
    char *fullname = palloc(strlen(first_given) + strlen(family) + 2);
    snprintf(fullname, "%s %s", first_given, family);
    text *result = cstring_to_text(fullname);
    PG_RETURN_TEXT_P(result);
}

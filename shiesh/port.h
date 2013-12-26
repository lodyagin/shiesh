#pragma once

long long
strtoll(const char *nptr, char **endptr, int base);

long long
strtonum(const char *numstr, long long minval, long long maxval,
    const char **errstrp);

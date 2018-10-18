#ifndef __YBN_H__
#define __YBN_H__ 1


typedef struct bn *ybn_t;

ybn_t ybn_alloc();
void ybn_free(ybn_t bn);


int ybn_add(ybn_t result, ybn_t a, ybn_t b);
int ybn_sub(ybn_t result, ybn_t a, ybn_t b);
int ybn_mul(ybn_t result, ybn_t a, ybn_t b);

int ybn_fromString(ybn_t bn, const char *s);
int ybn_toString(ybn_t bn, char *buf, int buflen);

int ybn_sqr(ybn_t result, ybn_t a);
int ybn_div(ybn_t quot, ybn_t rem, ybn_t a, ybn_t b);

int ybn_modexp(ybn_t result, ybn_t base, ybn_t exp, ybn_t modulus);

#endif // __YBN_H__

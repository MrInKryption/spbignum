#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "ybn.h"

#define UNDERGRAD

struct bn {
  int ybn_len;
  int ybn_size;
  int ybn_sign;
  uint16_t *ybn_data;
};


static int ybn_resize(ybn_t bn, int size) {
  if (size <= bn->ybn_size)
    return 0;
  if (size < bn->ybn_size * 2)
    size = bn->ybn_size * 2;
  uint16_t *data = (uint16_t *)realloc(bn->ybn_data, size * sizeof(uint16_t));
  if (data == NULL)
    return -1;
  for (int i = bn->ybn_size; i < size; i++)
    data[i] = 0;
  bn->ybn_data = data;
  bn->ybn_size = size;
  return 1;
}


static int ybn_reallen(ybn_t bn) {
  int l = bn->ybn_len;
  while (l-- > 0) {
    if (bn->ybn_data[l] != 0)
      return l+1;
  }
  return 0;
}

static void ydbn_push(ybn_t bn, uint8_t data) {
  uint32_t carry = data;
  for (int j = 0; j < bn->ybn_len; j++) {
    carry += bn->ybn_data[j] * 256;
    bn->ybn_data[j] = carry % 10000;
    carry = carry / 10000;
  }
  if (carry != 0)
    bn->ybn_data[bn->ybn_len++] = carry;
}

static ybn_t todec(ybn_t bn) {
  int binlen = ybn_reallen(bn);
  int declen = ((binlen + 3)/4) * 5;
  ybn_t dbn = ybn_alloc();
  if (dbn == NULL)
    return NULL;
  ybn_resize(dbn, declen);
  for (int i = binlen; i--; ) {
    ydbn_push(dbn, bn->ybn_data[i] >> 8);
    ydbn_push(dbn, bn->ybn_data[i] & 0xFF);
  }
  return dbn;
}
  

ybn_t ybn_alloc(void) {
  ybn_t bn = (ybn_t)malloc(sizeof(struct bn));
  if (bn == NULL)
    return NULL;
  bn->ybn_data = (uint16_t *)calloc(1, sizeof(uint16_t));
  if (bn->ybn_data == NULL) {
    free(bn);
    return NULL;
  }
  bn->ybn_len = 0;
  bn->ybn_size = 1;
  bn->ybn_sign = 1;
  return bn;
}


int ybn_toString(ybn_t bn, char *buf, int buflen) {
  ybn_t dbn = todec(bn);
  if (dbn == NULL)
    return -1;
  int dlen = dbn->ybn_len;
  uint16_t *data = dbn->ybn_data;

  int requiredlen;
  if (dlen == 0)
    requiredlen = 2;
  else
    requiredlen  = 2 + (bn->ybn_sign < 0) + (dlen - 1) * 4 +
	(data[dlen-1] > 999) + 
	(data[dlen-1] > 99) + 
	(data[dlen - 1] > 9);
  if (requiredlen > buflen) {
    ybn_free(dbn);
    return requiredlen;
  }

  char *p = buf;

  if (dlen == 0) {
    *p++ = '0';
  } else {
    if (bn->ybn_sign < 0)
      *p++ = '-';
    dlen--;
    if (data[dlen] > 999)
      *p++ = '0' + (data[dlen] / 1000) % 10;
    if (data[dlen] > 99)
      *p++ = '0' + (data[dlen] / 100) % 10;
    if (data[dlen] > 9)
      *p++ = '0' + (data[dlen] / 10) % 10;
    *p++ = '0' + data[dlen] % 10;
    while (dlen--) {
      *p++ = '0' + (data[dlen] / 1000) % 10;
      *p++ = '0' + (data[dlen] / 100) % 10;
      *p++ = '0' + (data[dlen] / 10) % 10;
      *p++ = '0' + (data[dlen] / 1) % 10;
    }
  }
  *p = '\0';
  ybn_free(dbn);
  return 0;
}




// shift n digits of b left by d bits and store the result in a.
// Assumptions: 
//    d < 16
//    a and b have space for n digits
//    The caller will handle setting the length
// Returns the carry from the shift
static uint16_t shiftleft(uint16_t *a, uint16_t *b, int n, int d) {
  uint32_t carry = 0;
  for (int i = 0; i < n; i++) {
    carry |= ((uint32_t)b[i]) << d;
    a[i] = carry & 0xffff;
    carry >>= 16;
  }
  return carry;
}
    

static uint32_t Step_D3(uint16_t *uj, uint16_t *v, int n) {
  uint32_t hat = (uj[n]<<16) + uj[n-1];
  uint32_t qhat = hat / v[n-1];
  uint32_t rhat = hat % v[n-1];
  if (qhat == 0x10000 || ( n>1 && (qhat * v[n-2] > 0x10000 * rhat + uj[n-2]))) {
    qhat--;
    rhat += v[n-1];
    if (rhat < 0x10000 && n>1 && (qhat * v[n-2] > 0x10000 * rhat + uj[n-2])) {
      qhat--;
      rhat += v[n-1];
    }
  }
  return qhat;
}

static uint16_t Step_D4(uint16_t *uj, uint16_t *v, uint32_t qhat, int n) {
  uint32_t borrow = 0;
  for (int i = 0; i < n; i++) {
    borrow += uj[i];
    borrow -= qhat * v[i];
    uj[i] = borrow & 0xFFFF;
    borrow >>= 16;
    if (borrow)
      borrow |= 0xFFFF0000; // The borrow is always non-positive...
  }
  borrow += uj[n];
  uj[n] = borrow & 0xFFFF;
  return borrow >> 16; // The return value is 16 bits, so no need for extending the sign bit
}

static void Step_D6(uint16_t *uj, uint16_t *v, int n) {
  uint32_t carry = 0;
  for (int i = 0; i < n; i++) {
    carry += uj[i];
    carry += v[i];
    uj[i] = carry & 0xFFFF;
    carry >>= 16;
  }
  carry += uj[n];
  uj[n] = carry & 0xFFFF;
}

static void shiftright(uint16_t *u, int n, int d) {
  for (int i = 0; i < n; i++)
    u[i] = (u[i] >> d) | (u[i+1] << (16 - d));
  u[n] >>= d;
}




// Using Algorithm 4.3.1 D of Knuth TAOCP
int ybn_div(ybn_t quotient, ybn_t remainder, ybn_t numerator, ybn_t denominator) {
  // Use Knuth's variable names:
  //   u -- numerator
  //   v -- denominator
  //   q -- quotient
  //   d -- normalisation factor
  //   n -- length of denominator
  //   m -- length difference between numerator and denominator
  //   b -- base = 0x10000
  // Our base (b) is 2^16, so we can use the shift method to calculate d.
  // We use the space of the remainder for the normalised numerator, so
  // need to allocate another variable for that.
  if (numerator->ybn_sign < 0 || denominator->ybn_sign < 0)
    return -1;
  if (quotient == numerator || 
      quotient == denominator || 
      quotient == remainder ||
      remainder == numerator ||
      remainder == denominator)
    return -1;

  // Step D1
  int n = ybn_reallen(denominator);
  if (n == 0)
    return -1;
  int d = 0;
  uint16_t t = denominator->ybn_data[n - 1];
  assert(t != 0); // This is OK from the calculation of n
  while ((t & 0x8000) == 0) {
    t <<= 1;
    d++;
  }
  ybn_t vbn = ybn_alloc();
  ybn_resize(vbn, n);
  uint16_t *v = vbn->ybn_data;
  t = shiftleft(v, denominator->ybn_data, n, d);
  // Not setting len of vbn because we do not really use it.
  assert(t == 0);

  int nl =  ybn_reallen(numerator);
  int m = nl < n ? 0 : nl - n;

  remainder->ybn_len = n;
  ybn_t ubn = remainder;
  ybn_resize(ubn, m + n + 1);
  memset(ubn->ybn_data, 0, (m + n + 1) * sizeof(uint16_t));
  uint16_t *u = ubn->ybn_data;
  ubn->ybn_data[nl] = shiftleft(u, numerator->ybn_data, nl, d);

  ybn_resize(quotient, m + 1);
  quotient->ybn_len = m + 1;
  uint16_t *q = quotient->ybn_data;


  // Steps D2, D7
  for (int j = m; j >= 0; j--) {
    // Step D3
    uint32_t qhat = Step_D3(u+j, v, n);

    // Step D4
    uint16_t borrow = Step_D4(u+j, v, qhat, n);
    
    // Step D5
    q[j] = qhat;
    if (borrow) {
      //Step D6
      assert(qhat != 0);
      Step_D6(u+j, v, n);
      q[j]--;
    }
  }
 
  // Step D8
  assert((u[0] & ((1<<d) - 1)) == 0);
  shiftright(u, n, d);
  ybn_free(vbn);

  return 0;
}




void ybn_free(ybn_t bn) {
  free(bn->ybn_data);
  free(bn);
}

  
static int ybn_muladd(ybn_t bn, int factor, int add) {
  int n = bn->ybn_len;
  uint16_t *d = bn->ybn_data;
  uint32_t carry = add;
  for (int i = 0; i < n; i++) {
    carry += d[i]  * factor;
    d[i] = (uint16_t)(carry & 0xFFFF);
    carry >>= 16;
  }
  if (carry) {
    ybn_resize(bn, n + 1);
    d = bn->ybn_data;
    d[n] = carry;
    bn->ybn_len = n+1;
  }
  return 0;
}


int ybn_fromString(ybn_t bn, const char *str) {
  const char *s = str;
  bn->ybn_sign = 1;
  bn->ybn_len = 0;
  if (*s == '-') {
    bn->ybn_sign = -1;
    s++;
  }
  int digit = 0;
  int factor = 1;
  while (isdigit(*s)) {
    digit = digit * 10 + *s - '0';
    factor *= 10;
    if (factor == 100000) {
      ybn_muladd(bn, factor, digit);
      factor = 1;
      digit = 0;
    }
    s++;
  }
  if (factor > 1)
    ybn_muladd(bn, factor, digit);
  return 0;
}

int ybn_mul(ybn_t result, ybn_t a, ybn_t b) {
  ybn_resize(result, a->ybn_len + b->ybn_len + 1);
  result->ybn_len = a->ybn_len + b->ybn_len + 1;
  result->ybn_sign = a->ybn_sign * b->ybn_sign;
  memset(result->ybn_data, 0, sizeof(uint16_t)* result->ybn_len);
  uint16_t *ad = a->ybn_data;
  uint16_t *rd = result->ybn_data;
  int alen = a->ybn_len;
  int blen = b->ybn_len;

  for (int i = 0; i < blen; i++) {
    uint32_t bd = b->ybn_data[i];
    uint32_t carry = 0;
    for (int j = 0; j < alen; j++) {
      carry = carry + ad[j] * bd + rd[i+j];
      rd[i+j] = carry & 0xFFFF;
      carry >>= 16;
    }
    rd[alen + i] = carry;
  }
  return 0;
}



static int ybn_addmagnitude(ybn_t result, ybn_t a, ybn_t b) {
  int an = a->ybn_len;
  int bn = b->ybn_len;
  int n = an;
  if (n < bn)
    n = bn;
  if (ybn_resize(result, n + 1) < 0)
    return -1;
  uint16_t *ad = a->ybn_data;
  uint16_t *bd = b->ybn_data;
  uint16_t *rd = result->ybn_data;
  uint32_t carry = 0;
  for (int i = 0; i < n; i++) {
    if (i < an)
      carry += ad[i];
    if (i < bn)
      carry += bd[i];
    rd[i] = (uint16_t)(carry & 0xFFFF);
    carry >>= 16;
  }
  rd[n] = carry;
  result->ybn_len = n+carry;
  result->ybn_sign = 1;
  return 0;
}

static int ybn_submagnitude(ybn_t result, ybn_t a, ybn_t b) {
  int an = a->ybn_len;
  int bn = b->ybn_len;
  int n = an;
  if (n < bn)
    n = bn;
  if (ybn_resize(result, n + 1) < 0)
    return -1;
  uint16_t *ad = a->ybn_data;
  uint16_t *bd = b->ybn_data;
  uint16_t *rd = result->ybn_data;
  int32_t borrow = 0;
  for (int i = 0; i < n; i++) {
    if (i < an)
      borrow += (int32_t)ad[i];
    if (i < bn)
      borrow -= (int32_t)bd[i];
    rd[i] = (uint16_t)(borrow & 0xFFFF);
    borrow >>= 16;
  }
  uint32_t carry = 0;
  result->ybn_sign = borrow * 2 + 1;
  borrow &= 0xFFFF;
  for (int i = 0; i < n; i++) {
    carry += rd[i] ^ borrow;
    rd[i] = (uint16_t)(carry & 0xFFFF);
    carry >>= 16;
  }
  rd[n] = carry;
  result->ybn_len = n+carry;
  return 0;
}


int ybn_add(ybn_t result, ybn_t a, ybn_t b) {
  int rv;
#ifdef UNDERGRAD
  rv = ybn_addmagnitude(result, a, b);
#else
  if (a->ybn_sign == b->ybn_sign)
    rv = ybn_addmagnitude(result, a, b);
  else
    rv = ybn_submagnitude(result, a, b);
#endif
  if (rv == -1)
    return -1;
  result->ybn_sign *= a->ybn_sign;
  return 0;
}


int ybn_sub(ybn_t result, ybn_t a, ybn_t b) {
  int rv;
#ifdef UNDERGRAD
  rv = ybn_submagnitude(result, a, b);
#else
  if (a->ybn_sign == b->ybn_sign)
    rv = ybn_submagnitude(result, a, b);
  else
    rv = ybn_addmagnitude(result, a, b);
#endif
  if (rv == -1)
    return -1;
#ifdef UNDERGRAD
  if (result->ybn_sign < 0) {
    result->ybn_len = 0;
    result->ybn_sign = 1;
  }
#endif
  result->ybn_sign *= a->ybn_sign;
  return 0;
}



int ybn_sqr(ybn_t result, ybn_t a) {
  ybn_resize(result, a->ybn_len * 2 + 2);
  result->ybn_len = a->ybn_len * 2 + 2;
  result->ybn_sign = 1;
  memset(result->ybn_data, 0, result->ybn_len);
  uint16_t *ad = a->ybn_data;
  int alen = a->ybn_len;
  uint16_t *rd = result->ybn_data;

  for (int i = 0; i < alen; i++) {
    uint64_t carry = rd[i*2] + (uint64_t)ad[i] * ad[i];
    rd[i * 2] = carry & 0xFFFFULL;
    carry >>= 16;
    for (int j = i + 1; j < alen; j++) {
      carry += 2 * (uint64_t)ad[i] * ad[j] + rd[i+j];
      rd[i+j] = carry & 0xFFFFULL;
      carry >>= 16;
    }
    carry += rd[i+alen];
    rd[i+alen] = carry & 0xFFFF;
  }
  return 0;
}


int ybn_modexp(ybn_t result, ybn_t base, ybn_t exp, ybn_t modulus) {
  ybn_resize(result, 1+ybn_reallen(modulus));
  result->ybn_len = 1;
  result->ybn_data[0] = 1;
  ybn_t tmp = ybn_alloc();
  ybn_t quot = ybn_alloc();

  for (int i = ybn_reallen(exp); i-- > 0; ) {
    for (int j = 15; j-- > 0;) {
      ybn_sqr(tmp, result);
      ybn_div(quot, result, tmp, modulus);
      if (exp->ybn_data[i] & (1 << j)) {
	ybn_mul(tmp, result, base);
	ybn_div(quot, result, tmp, modulus);
      }
    }
  }
  ybn_free(tmp);
  ybn_free(quot);
  return 0;
}


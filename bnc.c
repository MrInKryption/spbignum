//Raymond Habis a1631834

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "bn.h"
#include "ybn.h"

struct bn {
  int bn_len;
  int bn_size;
  int bn_sign;
  uint16_t *bn_data;
};
struct node{
    bn_t data;
    struct node* next;
};

struct yNode{
  ybn_t data;
  struct yNode* next;
};

struct node* newNode(bn_t data){
  struct node* stackNode=(struct node*) malloc(sizeof(struct node));
  stackNode->data=data;
  stackNode->next=NULL;
  return stackNode;
}

struct yNode *newYnode(ybn_t data){
  struct yNode* yStackNode=(struct yNode*)malloc(sizeof(struct yNode));
  yStackNode->data=data;
  yStackNode->next=NULL;
  return yStackNode;
}

struct yNode* yPush(struct yNode* yRoot, ybn_t yData){
  struct yNode* yStackNode=newYnode(yData);
  yStackNode->next=yRoot;
  yRoot=yStackNode;
  return yRoot;
}
ybn_t yPop(struct yNode** yRoot){
  if(yRoot==NULL){
    fprintf(stderr, "No value\n");
    exit(-1);
  }
  struct yNode* temp = *yRoot;
  *yRoot=(*yRoot)->next;
  ybn_t yPopped = temp->data;
  free(temp);
  return yPopped;
}

void yClear(struct yNode* yRoot){
  while(yRoot!=NULL){
    yPop(&yRoot);
  }
}
struct yNode* yDup(struct yNode* yRoot){
  if(yRoot==NULL){
    fprintf(stderr,"Error Null\n");
    exit(-1);
  }
  struct yNode* newYHead=newYnode(yRoot->data);
  newYHead->next=yRoot;
  return newYHead;
}


void yAdd(struct yNode* yRoot){
  ybn_t num1=yPop(&yRoot);
  ybn_t num2=yPop(&yRoot);
  ybn_t res=bn_alloc();
  ybn_add(res,num1,num2);
  yRoot=yPush(yRoot,res);
}


void ySub(struct yNode* yRoot){
  ybn_t num1=yPop(&yRoot);
  ybn_t num2=yPop(&yRoot);
  ybn_t res=bn_alloc();
  ybn_sub(res,num2,num1);
  yRoot=yPush(yRoot,res);
}

void yMul(struct yNode* yRoot){
  ybn_t num1=yPop(&yRoot);
  ybn_t num2=yPop(&yRoot);
  ybn_t res=bn_alloc();
  ybn_mul(res,num1,num2);
  yRoot=yPush(yRoot,res);
}


struct node* dup(struct node* root){
  if(root==NULL){
    fprintf(stderr,"Error Null\n");
    exit(-1);
  }
  struct node* newHead=newNode(root->data);
  newHead->next=root;
  return newHead;
}

struct node* swap(struct node* root){
  if(root==NULL){
    fprintf(stderr,"Error: Empty");
    exit(-1);
  }
  struct node* temp=root->next;
  if(temp==NULL){
    fprintf(stderr,"Error: Empty");
    exit(-1);
  }
  root->next=temp->next;
  temp->next=root;
  return temp;
}

void print(struct node* root){
  char buff[100000];
  //int size=bn_toString(root->data,buff,-1);
  //char newBuff[size];
  bn_toString(root->data,buff,sizeof(buff));
  printf("%s\n",buff);
}
// void yPrint(struct yNode* yRoot){
//   char buff[1];
//   int size=ybn_toString(yRoot->data,buff,-1);
//   char newBuff[size];
//   ybn_toString(yRoot->data,newBuff,size);
//   printf("%s\n",newBuff);
// }

void dump(struct node* root){
  while(root!=NULL){
    print(root);
    root=root->next;
  }
}

void yDump(struct yNode* yRoot){
  while(yRoot!=NULL){
    //yPrint(yRoot);
    yRoot=yRoot->next;
  }
}
 
struct node* push(struct node* root, bn_t data){
  struct node* stackNode=newNode(data);
  stackNode->next=root;
  root = stackNode;
  return root;
}

bn_t pop(struct node** root){
  if(*root==NULL){
    fprintf(stderr, "No value\n");
    exit(-1);
  }
  struct node* temp = *root;
  if((*root)->next==NULL){
      fprintf(stderr, "Empty at this point in the stack\n");
      exit(-1);
  }
  *root=(*root)->next;
  bn_t popped = temp->data;
  free(temp);
  return popped;
}

void clear(struct node* root){
  while(root!=NULL){
    pop(&root);
  }
}

void bn_free(bn_t bn){
  free(bn->bn_data);
  free(bn);
}


void add(struct node* root){
  bn_t num1=pop(&root);
  bn_t num2=pop(&root);
  bn_t res=bn_alloc();
  bn_add(res,num1,num2);
  root=push(root,res);
}


void sub(struct node* root){
  bn_t num1=pop(&root);
  bn_t num2=pop(&root);
  bn_t res=bn_alloc();
  bn_sub(res,num2,num1);
  root=push(root,res);
}

void mul(struct node* root){
  bn_t num1=pop(&root);
  bn_t num2=pop(&root);
  bn_t res=bn_alloc();
  bn_mul(res,num1,num2);
  root=push(root,res);
}

static int bn_resize(bn_t bn, int size) {
  if (size <= bn->bn_size)  
    return 0;
  uint16_t *data = (uint16_t *)realloc(bn->bn_data, size * sizeof(uint16_t));
  if (data == NULL)
    return -1;
  for (int i = bn->bn_size; i < size; i++)
    data[i] = 0;
  bn->bn_data = data;
  bn->bn_size = size;
  return 1;
}


static int bn_reallen(bn_t bn) {
  int l = bn->bn_len;
  while (l-- > 0) {
    if (bn->bn_data[l] != 0)
      return l+1;
  }
  return 0;
}

static void dbn_push(bn_t bn, uint8_t data) {
  uint32_t carry = data;
  for (int j = 0; j < bn->bn_len; j++) {
    carry += bn->bn_data[j] * 256;
    bn->bn_data[j] = carry % 10000;
    carry = carry / 10000;
  }
  if (carry != 0)
    bn->bn_data[bn->bn_len++] = carry;
}

static bn_t todec(bn_t bn) {
  int binlen = bn_reallen(bn);
  int declen = ((binlen + 3)/4) * 5;
  bn_t dbn = bn_alloc();
  if (dbn == NULL)
    return NULL;
  bn_resize(dbn, declen);
  for (int i = binlen; i--; ) {
    dbn_push(dbn, bn->bn_data[i] >> 8);
    dbn_push(dbn, bn->bn_data[i] & 0xFF);
  }
  return dbn;
}
  

bn_t bn_alloc(void) {
  bn_t bn = (bn_t)malloc(sizeof(struct bn));
  if (bn == NULL)
    return NULL;
  bn->bn_data = (uint16_t *)calloc(1, sizeof(uint16_t));
  if (bn->bn_data == NULL) {
    free(bn);
    return NULL;
  }
  bn->bn_len = 0;
  bn->bn_size = 1;
  bn->bn_sign = 1;
  return bn;
}


int bn_toString(bn_t bn, char *buf, int buflen) {
  bn_t dbn = todec(bn);
  if (dbn == NULL)
    return -1;
  int dlen = dbn->bn_len;
  uint16_t *data = dbn->bn_data;


  int requiredlen;
  if (dlen == 0)
    requiredlen = 2;
  else
    requiredlen  = 2 + (bn->bn_sign < 0) + (dlen - 1) * 4 +
	(data[dlen-1] > 999) + 
	(data[dlen-1] > 99) + 
	(data[dlen - 1] > 9);
  if (requiredlen > buflen) {
    bn_free(dbn);
    return requiredlen;
  }

  char *p = buf;

  if (dlen == 0) {
    *p++ = '0';
  } else {
    if (bn->bn_sign < 0)
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
  bn_free(dbn);
  return 0;
}

int bn_fromString(bn_t bn, const char *s){
  int length=strlen(s);
  if(length==0){
    return -1;
  }
  int len=length-1;
  int i = 0;
  int j = 0;
  int base=65536;
  int digit;
  int tmp;
  int count=0;
  uint32_t *res=(uint32_t *)calloc(length, sizeof(uint32_t));
  for(i = 0; i<length; i++){
    for(int j=0;j<length;j++){
      res[j]*=10;
    }
    digit = s[i]-'0';
    tmp=res[0]+digit;
    res[0]=tmp%base;
    if(length>1){
        res[1]+=tmp/base;
        for(j=1;j<length-1;j++){
        tmp=res[j];
        res[j]=tmp%base;
        res[j+1]+=tmp/base;
        }
    }
  }
  while(res[len]==0){
    count++;
    len--;
  }

  int diff=length-count;
  bn->bn_len=diff;
  bn_resize(bn,bn->bn_len);
  for(i=0;i<diff;i++){
    bn->bn_data[i]=res[i];
    if(bn->bn_data[i]<0){
      return -1;
    }
  }
  free(res);
  return 0;
}


int bn_add(bn_t result, bn_t a, bn_t b){
  int i=0;
  int base=65536;
  int size;
  int carry=0;
  uint32_t sum=0;
  if(a->bn_len>=b->bn_len){
    bn_resize(b, a->bn_len);
    size=a->bn_len;
  }
  else if(b->bn_len>=a->bn_len){
    bn_resize(a, b->bn_len);
    size=b->bn_size;
  }
  bn_resize(result,size+1);
  result->bn_len=a->bn_len+1;
  for(i=0;i<size;i++){
    sum+=a->bn_data[i]+b->bn_data[i];
    if(sum>=base){
      carry=1;
      sum=sum-base;
      result->bn_data[i]=sum;
      sum=carry;
    } else {
      carry=0;
      result->bn_data[i]=sum;
      sum=carry;
    }
  }
  result->bn_data[size]=sum;

  return 0;
}

int bn_sub(bn_t result, bn_t a, bn_t b){
  int i=0;
  int base=65536;
  int size;
  int carry=0;
  uint32_t sum=0;
  if(a->bn_len>=b->bn_len){
    size=a->bn_len;
  }
  else if(b->bn_len>=a->bn_len){
    size=b->bn_len;
  }
  bn_resize(result,size+1);
  result->bn_len=a->bn_len+1;
  for(i=0;i<1;i++){
    if(a->bn_data[i]<=b->bn_data[i]){
      a->bn_data[i]=0;
      b->bn_data[i]=0;
    }
    sum-=b->bn_data[i]-a->bn_data[i];
    if(sum>=base){
      carry=1;
      sum=sum-base;
      result->bn_data[i]=sum;
      sum=carry;
    } else {
      carry=0;
      result->bn_data[i]=sum;
      sum=carry;
    }

  }
  result->bn_data[size]=sum;

  return 0;
}

int bn_mul(bn_t result, bn_t a, bn_t b){
  int i=0;
  int j=0;
  int k=0;
  int base=65536;
  int size;
  int sizeA=a->bn_len;
  int sizeB=b->bn_len;
  int carry=0;
  if(a->bn_len>=b->bn_len){
    size=a->bn_len;
  }
  else if(b->bn_len>=a->bn_len){
    size=b->bn_len;
  }
  uint32_t *prod=(uint32_t *)calloc(size*2, sizeof(uint32_t));
  result->bn_len=size*2;
  bn_resize(result,result->bn_len);
  for(i=0;i<sizeA;i++){
    for(j=0;j<sizeB;j++){
      carry=0;
      prod[i+j]+=a->bn_data[i]*b->bn_data[j]+carry;
      carry=prod[i+j]/base;
      prod[i+j]=prod[i+j]%base;
      prod[i+j+1]+=carry;
    }
  }

  for(k=0;k<size*2;k++){
    result->bn_data[k]=prod[k];
  }
  return 0;
}

int bnIAmAnUndergrad(){
  return 1;
}

int get_bn_size(bn_t num){
  char buff[1];
  int size=bn_toString(num,buff,-1);
  return size;
}

static uint32_t Step_D3(uint16_t *uj, uint16_t *v, int n) {
  uint32_t hat = (uj[n]<<16) + uj[n-1];
  uint32_t qhat = hat / v[n-1];
  uint32_t rhat = hat % v[n-1];
  while (qhat > 0x10000) {
    qhat--;
    rhat += v[n-1];
  }
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
static uint16_t Step_D4(uint16_t *uj, uint16_t *v, uint32_t qhat, int n){
  uint32_t borrow = 0;
  for (int i = 0; i < n; i++){
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

static void Step_D6(uint16_t *uj, uint16_t *v, int n){
  uint32_t carry = 0;
  for (int i = 0; i < n; i++) {
    carry += uj[i];
    carry += v[i];
    uj[i] = carry & 0xFFFF;
    carry >>= 16;
  }
  carry += uj[n];
  uj[n] = carry & 0xFFFF;
  //assert(carry > 0xFFFF); // We ignore further carry
}

static void shiftright(uint16_t *u, int n, int d) {
  for (int i = 0; i < n; i++){
    u[i] = (u[i] >> d) | (u[i+1] << (16 - d));
  }
  u[n] >>= d;
}
static uint16_t shiftleft(uint16_t *a, uint16_t *b, int n, int d){
  //printf("shiftleft(res, "); dump(b, n); printf(", %d, %d)\n", n, d);
  uint32_t carry = 0;
  for (int i = 0; i < n; i++){
    carry |= ((uint32_t)b[i]) << d;
    a[i] = carry & 0xffff;
    carry >>= 16;
  }
  //printf("shiftleft: %04X  + ", carry); dump(a, n); printf("\n");
  return carry;
}

// Using Algorithm 4.3.1 D of Knuth TAOCP
int bn_div(bn_t quotient, bn_t remainder, bn_t numerator, bn_t denominator) {
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
  if (numerator->bn_sign < 0 || denominator->bn_sign < 0){

    return -1;
  }
  if (quotient == numerator || 
      quotient == denominator || 
      quotient == remainder ||
      remainder == numerator ||
      remainder == denominator)
    return -1;

  // Step D1
  int n = bn_reallen(denominator);
  if (n == 0){
    return -1;
  }
  int d = 0;
  uint16_t t = denominator->bn_data[n - 1];
  assert(t != 0); // This is OK from the calculation of n
  while ((t & 0x8000) == 0){
    t <<= 1;
    d++;
  }
  bn_t vbn = bn_alloc();
  bn_resize(vbn, n);
  uint16_t *v = vbn->bn_data;
  t = shiftleft(v, denominator->bn_data, n, d);
  // Not setting len of vbn because we do not really use it.
  assert(t == 0);

  int nl =  bn_reallen(numerator);
  int m = nl < n ? 0 : nl - n;

  remainder->bn_len = n;
  bn_t ubn = remainder;
  bn_resize(ubn, m + n + 1);
  memset(ubn->bn_data, 0, (m + n + 1) * sizeof(uint16_t));
  uint16_t *u = ubn->bn_data;
  ubn->bn_data[nl] = shiftleft(u, numerator->bn_data, nl, d);

  bn_resize(quotient, m + 1);
  quotient->bn_len = m + 1;
  uint16_t *q = quotient->bn_data;


  // Steps D2, D7
  for (int j = m; j >= 0; j--){
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
  bn_free(vbn);

  return 0;
}

int exponentiate(bn_t result,bn_t exponent,bn_t base){
  int size=get_bn_size(base);
  char buffer[size];
  bn_toString(exponent,buffer,size);
  bn_t tempRemainder = bn_alloc();
  bn_t tempDenominator=bn_alloc();
  bn_t tempRes=bn_alloc();
  bn_t tempExponent=bn_alloc();
  bn_fromString(tempDenominator,"2");
  bn_t one=bn_alloc();
  bn_fromString(one,"1");

  if(strncmp(buffer, "0",size)==0){
    bn_fromString(result, "1");
    return 0;
  }

  else if(strncmp(buffer,"1",size)==0){
    bn_toString(base,buffer,size);
    bn_fromString(result,buffer);
    return 0;
  }
  else if(exponent->bn_data[0]%2==0){
    bn_mul(result, base, base);
    bn_div(tempExponent,tempRemainder,exponent,tempDenominator);
    bn_toString(tempExponent,buffer,size);
    bn_fromString(exponent,buffer);
    bn_free(tempDenominator);
    bn_free(tempRemainder);
    bn_free(tempExponent);
    exponentiate(result,exponent,result);
    return 0;
  }
  else if(exponent->bn_data[0]%2==1){
    bn_toString(base,buffer,size);
    bn_fromString(tempRes,buffer);
    bn_mul(result, base, base);
    bn_div(tempExponent,tempRemainder,exponent,tempDenominator);
    bn_toString(tempExponent,buffer,size);
    bn_fromString(exponent,buffer);
    bn_free(tempDenominator);
    bn_free(tempRemainder);
    bn_free(one);
    bn_free(tempExponent);
    exponentiate(result,exponent,result);
    bn_mul(result,tempRes,result);
    bn_free(tempRes);
    return 0;
  }
  return 0;
}

int bn_modexp(bn_t result, bn_t base, bn_t exp, bn_t modulus){
  int size=get_bn_size(base);
  char buffer[size];
  if(strncmp(buffer,"1",size)==0){
    bn_fromString(result, "0");
    return 0;
  }
  bn_t quotient=bn_alloc();
  bn_t tempRem=bn_alloc();
  exponentiate(result,exp,base);
  bn_div(quotient,tempRem,result,modulus);
  bn_toString(tempRem,buffer,size);
  bn_fromString(result,buffer);
  bn_free(tempRem);
  bn_free(quotient);
  return 0;
}

void modexp(struct node* root){
  if(root !=NULL){
  struct node* temp=root;
  struct node* temp2=root->next;
  struct node* temp3=root->next->next;
  //printf("%d\n",temp3->data->bn_data[0]);
   //bn_t num=pop(&temp);

   bn_t num3=pop(&temp3);
   //printf("%d\n",num3->bn_data[0]);
   bn_t num2=pop(&temp2);
    dump(temp);
  //bn_t res=bn_alloc();
  //bn_modexp(res,num,num2,num3);
  //root=push(root,res);
  }
}
// void yModExp(struct yNode* yRoot){
//   ybn_t base=yRoot->data;
//   ybn_t exp=yRoot->next->data;
//   ybn_t mod=yRoot->next->next->data;
//   ybn_t res=ybn_alloc();
//   ybn_modexp(res,base,exp,mod);
//   yRoot=yPush(yRoot,res);
// }

int main(int argc, char const *argv[]){
  char buff[50000];
  bn_t *a;
  ybn_t *b;
  FILE *ptr_file;
  ptr_file =fopen(argv[1],"r");
  if(ptr_file==NULL){
    fgets(buff,50000,stdin);
  } 
  else{
    fgets(buff,50000, ptr_file);
  }

  int i=0;
  struct node* root=NULL;
  struct yNode* yRoot=NULL;
  char *tokBuff=strtok(buff, " \t\n\v\f\r");
  while(tokBuff!=NULL){
    if(strcmp(tokBuff,"+")==0){
      add(root);
      yAdd(yRoot);
    }
    else if(strcmp(tokBuff,"-")==0){
      sub(root);
      ySub(yRoot);
    }
    else if(strcmp(tokBuff,"*")==0){
      mul(root);
      yMul(yRoot);
    }
    else if(strcmp(tokBuff,"dump")==0){
      dump(root);
      yDump(yRoot);
    }
    else if(strcmp(tokBuff,"clear")==0){
      clear(root);
      yClear(yRoot);
    }
    else if(strcmp(tokBuff,"swap")==0){
      root=swap(root);
    }
    else if(strcmp(tokBuff,"dup")==0){
      root=dup(root);
      yRoot=yDup(yRoot);
    }
    else if(strcmp(tokBuff,"pop")==0){
      pop(&root);
      yPop(&yRoot);
    }
    else if(strcmp(tokBuff,"print")==0){
      print(root);
      //yPrint(yRoot);
    }
    else if(strcmp(tokBuff, "#")==0){
      modexp(root);
      //yModExp(yRoot);
    }
    else{
      while(tokBuff[i]!='\0'){
        if((tokBuff[i]>='0' && tokBuff[i]<='9') ||tokBuff[i]=='\0'){
          i++;
        } else{
          printf("%c\n",tokBuff[i]);
          return -1;
        }
      }
      a=malloc(sizeof(bn_t));
      b=malloc(sizeof(ybn_t));
      *a=bn_alloc();
      *b=ybn_alloc();
      bn_fromString(*a,tokBuff);
      ybn_fromString(*b,tokBuff);
      root=push(root,*a);
      //yRoot=yPush(yRoot,*b);
    }
    i=0;
    tokBuff=strtok(NULL, " \t\n\v\f\r");
  }
  bn_free(*a);
  ybn_free(*b);

   return 0;
  
}

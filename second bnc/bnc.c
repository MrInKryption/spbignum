//Raymond Habis a1631834

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "bn.h"

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


struct node* newNode(bn_t data){
  struct node* stackNode=(struct node*) malloc(sizeof(struct node));
  stackNode->data=data;
  stackNode->next=NULL;
  return stackNode;
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
  char buff[1];
  int size=bn_toString(root->data,buff,-1);
  char newBuff[size];
  bn_toString(root->data,newBuff,size);
  printf("%s\n",newBuff);
}

void dump(struct node* root){
  while(root!=NULL){
    print(root);
    root=root->next;
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

int main(int argc, char const *argv[])
{
  char buff[50000];
  bn_t *a;
  int i=0;
  fgets(buff,50000,stdin);
  struct node* root=NULL;
  char *tokBuff=strtok(buff, " \t\n\v\f\r");
  while(tokBuff!=NULL){
    if(strcmp(tokBuff,"+")==0){
      add(root);
    }
    else if(strcmp(tokBuff,"-")==0){
      sub(root);
    }
    else if(strcmp(tokBuff,"*")==0){
      mul(root);
    }
    else if(strcmp(tokBuff,"dump")==0){
      dump(root);
    }
    else if(strcmp(tokBuff,"clear")==0){
      clear(root);
    }
    else if(strcmp(tokBuff,"swap")==0){
      root=swap(root);
    }
    else if(strcmp(tokBuff,"dup")==0){
      root=dup(root);
    }
    else if(strcmp(tokBuff,"pop")==0){
      pop(&root);
    }
    else if(strcmp(tokBuff,"print")==0){
      print(root);
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
      *a=bn_alloc();
      bn_fromString(*a,tokBuff);
      root=push(root,*a);  
    }
    i=0;
    tokBuff=strtok(NULL, " \t\n\v\f\r");
  }
  bn_free(*a);

   return 0;
}

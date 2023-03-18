#include <assert.h>
#include "compiler.h"

int E();
void STMT();
void IF();
void BLOCK();

int tempIdx = 0, labelIdx = 0;

#define nextTemp() (tempIdx++)
#define nextLabel() (labelIdx++)
// #define emit printf


int isTempIr = 0;
char tempIr[100000], *tempIrp = tempIr;
#define emit(...) ({ \
  if (isTempIr) { \
    sprintf(tempIrp, __VA_ARGS__); \
    tempIrp += strlen(tempIrp);\
  } else { \
    printf(__VA_ARGS__);\
  }\
})

// for (assign E;E) STMT
FOR() {
  skip("for");
  skip("(")
  ASSIGN()
  int e2 = E();
  skip(";")
  isTempIr = 1
  int e3 = E()
  isTempIr = 0
  char e3str[1000];
  strcpy(e3str, tempIr)
  skip(")")
  STMT()
  emit("%s\n", e3str)
}

// do STMT while (E)
DO_WHILE() {
  skip("do");
  int doBegin = nextLabel();
  emit("(L%d)\n", doBegin);
  STMT();
  skip("while");
  skip("(");
  int e = E();
  skip(")");
  emit("if T%d goto L%d\n", e, doBegin);
}

int isNext(char *set) {
  char eset[SMAX], etoken[SMAX];
  sprintf(eset, " %s ", set);
  sprintf(etoken, " %s ", tokens[tokenIdx]);
  return (tokenIdx < tokenTop && strstr(eset, etoken) != NULL);
}

int isEnd() {
  return tokenIdx >= tokenTop;
}

char *next() {
  // printf("token[%d]=%s\n", tokenIdx, tokens[tokenIdx]);
  return tokens[tokenIdx++];
}

char *skip(char *set) {
  if (isNext(set)) {
    return next();
  } else {
    printf("skip(%s) got %s fail!\n", set, next());
    assert(0);
  }
}

// F = (E) | Number | Id
int F() {
  int f;
  if (isNext("(")) { // '(' E ')'
    next(); // (
    f = E();
    next(); // )
  } else { // Number | Id
    f = nextTemp();
    char *item = next();
    if (isdigit(*item)) {
      emit("t%d = %s\n", f, item);
    } else {
      if (isNext("++")) {
        next();
        emit("%s = %s + 1\n", item, item);
      }
      emit("t%d = %s\n", f, item);
    }
  }
  return f;
}

// E = F (op E)*
int E() {
  int i1 = F();
  while (isNext("+ - * / & | ! < > = <= >= == !=")) {
    char *op = next();
    int i2 = E();
    int i = nextTemp();
    emit("t%d = t%d %s t%d\n", i, i1, op, i2);
    i1 = i;
  }
  return i1;
}

// FOR =  for (ASSIGN EXP; EXP) STMT

// ASSIGN = id '=' E;
void ASSIGN() {
  char *id =

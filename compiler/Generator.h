typedef struct
{
    HashTable*symTable;
    Tree*tree;
    FILE*asmFile;
    int forCount, varCount;
}Generator;

void generate(Tree*tree,char*asmFile);

Generator*GenNew();
void GenFree(...);
Tree*GenCode(...);
void GenData(...);
void GenPcode(...);
void GenPcodeToAsm(...);
void GenAsmCode(...);
void GenTempVar(...);
void negateOp(...);
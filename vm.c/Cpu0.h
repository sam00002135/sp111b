typedef struct
{
    BYTE *m;
    int mSize;
    int R[16], IR;
} Cpu0;

void runObjFile(char *ObjFile);

Cpu0* Cpu0new(char *ObjFile);
void Cpu0Free(Cpu0 *cpu0);
CpuRun(Cpu0 *cpu0, int startPC);
void Cpu0Dump(Cpu *cpu0);
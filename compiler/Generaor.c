void GenPcode(Generator *g,char* label,char*op,char*p1,char*p2,char*pTo){
    char labelTemp[100];
    if(strlen(label)>0)
        sprintf(labelTemp,"%s:",label);
    else
        strcpy(labelTemp,"");
    printf("%-8s %-4s %-4s %-4s %-4s\n",labelTemp,op,p1,p2,pTo);
}

void GenPcodeToAsm(Generator *g,char* label,char*op,char*p1,char*p2,char*pTo){
    if(strlen(label)>0)
        GenAsmCode(g,label,"","","","");
    if(strEqual(op,"=")){
        GenAsmCode(g, "", "LD","R1",p1,"");
        GenAsmCode(g, "", "ST","R1",pTo,"");
    }else if(strPartOf(op,"|+|-|*|/|")){
        char asmOp[100];
        if(strEqual(op,"+")) strcpy(asmOp,"ADD");
        else if(strEqual(op,"-")) strcpy(asmOp,"SUB");
        else if(strEqual(op,"*")) strcpy(asmOp,"MUL");
        else if(strEqual(op,"/")) strcpy(asmOp,"DIV");
        GenAsmCode(g, "","LD","R1",P1,"");
        GenAsmCode(g, "","LD","R2",P2,"");
        GenAsmCode(g, "",asmOp,"R3","R2","R1");
        GenAsmCode(g, "","ST","R3",PTo,"");
    }else if(strEqual(op,"CMP")){
        GenAsmCode(g, "","LD","R1",p1,"");
        GenAsmCode(g, "","LD","R2",p2,"");
        GenAsmCode(g, "","CMP","R1","R2","");
    }else if(strEqual(op,"j")){
        char asmOp[100];
        if(strEqual(p1,"=")) strcpy(asmOp,"JEQ");
        else if(strEqual(p1,"!=")) strcpy(asmOp,"JNE");
        else if(strEqual(p1,"<")) strcpy(asmOp,"JLT");
        else if(strEqual(p1,">")) strcpy(asmOp,"JGT");
        else if(strEqual(p1,"<=")) strcpy(asmOp,"JLE");
        else if(strEqual(p1,">=")) strcpy(asmOp,"JGE");
        else strcpy(asmOp,"JMP");
        GenAsmCode(g, "", asmOp,pTo,"","");
    }else if(strEqual(op,"RET")){
        GenAsmCode(g, "","LD","R1",pTo,"");
        GenAsmCode(g, "","RET","","","");
    }
}

void GenAsmCodeCode(Generator *g,char* label,char* op,char*p1,char*p2,char*pTo){
    char* realOp = op;
    if(strEqual(op,"LD"))
        if(isdigit(p2[0]))
            realOp = "LDI";
    fprintf(g->asmFile, "%-8s %-4s %-4s %-4s %-4s\n",label,realOp,p1,p2,pTo);
}
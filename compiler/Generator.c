void generate(Tree*tree,char*asmFile)
{
    char nullVar[100]="""";
    Generator *g = GenNew();
    g->asm = fopen(asmFile,"w");
    printf("=====PCODE=====\n");
    GenData(g);
    fclose(g->asmFile);
    GenFree(g);
    char*asmText = newFileStr(asmFile);
    printf("=====AsmFile:%s=====\n",asmFile);
    printf("%s\n",asmText);
    freeMemory(asmText);
}

Tree* GenCode(Generator*g, Tree*node,char*rzVar)
{
    strcpy(nullVar,"");
    strcpy(rzVar,"");
    if(node == NULL)return NULL;
    if(strEqual(node->type,"FOR")){
        char forBeginLabel[100],forEndLabel[100],condOp[100];
        Tree *stmt1 = node->childs->item[2];
            *cond = node->childs->item[4];
            *stmt2 = node->childs->item[6];
            *block = node->childs->item[8];
        GenCode(g, stmt1, nullVar);
        int tempForCount = g->forCount++
        sprintf(forBeginLabel,"FOR%d",tempForCount);
        sprintf(forEndLabel,"_FOR%d",tempForCount);
        GenPcode(g, forBeginLabel,"","","","");
        GenCode(g, cond, condOp);
        char negOp[100];
        negateOp(condOp,negOp);
        GenPcode(g,"","j",negOp,"",forEndLabel);
        GenCode(g, block, nullVar);
        GenCode(g, stmt2, nullVar);
        GenPcode(g,"","j","","",forBeginLabel);
        GenPcode(g,forEndLabel,"","","","");
        return NULL;
    }else if(strEqual(node->type,"STMT")){
        Tree*c1 = node->childs->item[0];
        if(strEqual(c1->type,"return")){
            Tree*id = node->childs->item[1];
            GenPcode(g,"","RET","","",id->value);
        }else{
            Tree *id = node->childs->item[0];
            Tree *op = node->childs->item[1];
            if(strEqual(op->type,"=")){
                Tree*exp = node->childs->item[2];
                char expVar[100];
                GenCode(g, exp, expVar);
                GenPcode(g, "","=",expVar,"",id->value);
                HashTablePut(g->symTable,id->value,id->value);
                strcpy(rzVar,expVar);
            }else{
                char addsub[100];
                if(strEqual(op->value,"++"))
                    strcpy(addsub,"+");
                else
                    strcpy(addsub,"-");
                GenPcode(g,"",addsub,id->value,"1",id->value);
                strcpy(rzVar,id->value);
            }
        }
    }else if (strEqual(node->type,"COND")){
        Tree* op = node->childs->item[1];
        char expVar1[100],expVar2[100];
        GenCode(g,node->childs->item[0],expVar1);
        GenCode(g,node->childs->item[2],expVar2);
        GenPcode(g,"","CMP",expVar1,expVar2,nullVar);
        strcpy(rzVar, op->value);
    }else if (strPartOf(node->type,"|EXP|")){
    }else if (strPartOf(node->type,"|EXP|")){
        Tree *item1 = node->childs->item[0]
        char var1[100],var2[100],tempVar[100];
        GenCode(g, item1, var1);
        if(node->childs->count>1){
            Tree* op = node->childs->item[1];
            Tree* item2 = node->childs->item[2];
            GenCode(g, item2, var2);
            GenTempVar(g, tempVar);
            GenPcode(g, "",op->value,var1,var2,tempVar);
            strcpy(var1,tempVar);
        }
        strcpy(rzVar, var1);
    }else if (strPartOf(node->type,"|number|id|")){
        strcpy(rzVar, node->value);
    }else if (node->childs!=NULL){
        int i;
        for(i=0; i< node->childs->count; i++)
        GenCode(g, node->childs->items[i],nullVar);
    }
    return NULL;
}
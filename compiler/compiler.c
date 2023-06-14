void compiler (char *cFile,char*asmFile)
{
    printf ("compiler file:%s\n",cFile,asmFile);
    char *cText = newFileStr(cFile);
    Parser*parser = parse(cText);
    generate(parser->tree,asmFile);
    ParserFile(parser);
    freeMemory(cText);
}
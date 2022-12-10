#define AK_JSON_IMPLEMENTATION
#include "ak_json.h"

#include "utest.h"

ak_json_str AK_Json_Test_Read(const char* Path)
{
    ak_json_str Result;
    Result.Str = NULL;
    Result.Length = 0;
    
    FILE* File = fopen(Path, "r");
    if(!File) return Result;
    
    fseek(File, 0, SEEK_END);
    Result.Length = ftell(File);
    fseek(File, 0, SEEK_SET);
    
    ak_json_u8* Buffer = (ak_json_u8*)malloc(Result.Length+1);
    Buffer[Result.Length] = 0;
    
    Result.Str = (const ak_json_u8*)Buffer;
    
    fread(Buffer, Result.Length, 1, File);
    
    fclose(File);
    
    return Result;
}

static void* AK_Json_Test_Null_Allocate(ak_json_allocator* Allocator, unsigned int Size)
{
    Allocator = Allocator;
    Size = Size;
    return NULL;
}

static void AK_Json_Test_Free_Stub(ak_json_allocator* Allocator, void* Memory)
{
    Allocator = Allocator;
    Memory = Memory;
}

UTEST(AK_Json, OutOfMemory)
{
    ak_json_allocator Allocator;
    Allocator.Allocate = AK_Json_Test_Null_Allocate;
    Allocator.Free = AK_Json_Test_Free_Stub;
    
    ak_json_context* Context = AK_Json_Create(&Allocator);
    ASSERT_EQ(Context, NULL);
    ASSERT_EQ(AK_Json_Get_Error_Code(), AK_JSON_ERROR_CODE_OUT_OF_MEMORY);
}

UTEST(AK_Json, Simple_Success)
{
    ak_json_context* Context = AK_Json_Create(NULL);
    
    ak_json_str Json0  = AK_Json_Str("0");
    ak_json_str Json1  = AK_Json_Str("14");
    ak_json_str Json2  = AK_Json_Str("0.1");
    ak_json_str Json3  = AK_Json_Str("-10.23");
    ak_json_str Json4  = AK_Json_Str("14e4");
    ak_json_str Json5  = AK_Json_Str("14e-4");
    ak_json_str Json6  = AK_Json_Str("-0.2e-4");
    ak_json_str Json7  = AK_Json_Str("0e+4");
    ak_json_str Json8  = AK_Json_Str("null");
    ak_json_str Json9  = AK_Json_Str("true");
    ak_json_str Json10 = AK_Json_Str("false");
    ak_json_str Json11 = AK_Json_Str("\"\"");
    ak_json_str Json12 = AK_Json_Str("\"/\"");
    ak_json_str Json13 = AK_Json_Str("\"\\\"\"");
    ak_json_str Json14 = AK_Json_Str("\"\\uabcd\"");
    ak_json_str Json15 = AK_Json_Str("\"Hello \\t \\uabcd World \\b\\n \\u15fc My name\"");
    ak_json_str Json16 = AK_Json_Str("\"Hello \\uabcd\\n World \\u15fc My \\r name\"");
    
    ASSERT_FALSE(AK_Json_Parse(Context, Json0) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json1) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json2) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json3) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json4) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json5) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json6) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json7) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json8) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json9) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json10) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json11) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json12) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json13) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json14) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json15) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json16) == NULL);
    
    AK_Json_Delete(Context);
}

UTEST(AK_Json, Simple_Array)
{
    ak_json_context* Context = AK_Json_Create(NULL);
    
    ak_json_str Json0 = AK_Json_Str("[]");
    ak_json_str Json1 = AK_Json_Str("[123]");
    ak_json_str Json2 = AK_Json_Str("[\"\\uabcd\", 123, null, false, -0.2e4]");
    
    ASSERT_FALSE(AK_Json_Parse(Context, Json0) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json1) == NULL);
    ASSERT_FALSE(AK_Json_Parse(Context, Json2) == NULL);
    
    AK_Json_Delete(Context);
}

UTEST(AK_Json, Simple_Error)
{
    ak_json_context* Context = AK_Json_Create(NULL);
    
    ak_json_str Json0 = AK_Json_Str("01");
    ASSERT_EQ(AK_Json_Parse(Context, Json0), NULL);
    
    printf("%s", (const char*)AK_Json_Get_Error_Message().Str);
    
    AK_Json_Delete(Context);
}

#if 0 
UTEST(AK_Json, Simple_Error)
{
    ak_json_context* Context = AK_Json_Create(NULL);
    
    ak_json_str EmptyStr = AK_Json_Str("");
    ak_json_str InvalidStr0 = AK_Json_Str("Blah");
    ak_json_str InvalidStr1 = AK_Json_Str("\"Ha");
    ak_json_str InvalidStr2 = AK_Json_Str("{\n"+
                                          "\t\"Hello\": {\n"+
                                          "\t\t\"ha\n"+
                                          "\t}"+
                                          "}");
    
    ASSERT_EQ(AK_Json_Parse(Context, EmptyStr), NULL);
    ASSERT_EQ(AK_Json_Get_Error_Code(), AK_JSON_ERROR_CODE_EMPTY_STRING);
    
    ak_json_str ErrorMessage = AK_Json_Str("Error: Cannot parse empty string");
    ASSERT_EQ(strcmp(AK_Json_Get_Error_Message().Str, ErrorMessage.Str), 0);
    
    ASSERT_EQ(AK_Json_Parse(Context, InvalidStr0), NULL);
    ASSERT_EQ(AK_Json_Get_Error_Code(), AK_JSON_ERROR_CODE_UNDEFINED);
    
    ErrorMessage = AK_Json_Str("Error: Expecting a string, number, null, true, false, object, or an array. Got undefined\n"+
                               "1 Blah\n"+
                               "  ^\n");
    ASSERT_EQ(strcmp(AK_Json_Get_Error_Message().Str, ErrorMessage.Str), 0);
    
    ASSERT_EQ(AK_Json_Parse(Context, InvalidStr1), NULL);
    ASSERT_EQ(AK_Json_Get_Error_Code(), AK_JSON_ERROR_CODE_INVALID_STRING);
    
    ErrorMessage = AK_Json_Str("Error: Invalid string. Missing closing quotes\n"+
                               "1 \"Ha\n"+
                               "   ^\n");
    ASSERT_EQ(strcmp(AK_Json_Get_Error_Message().Str, ErrorMessage.Str), 0);
    
    ASSERT_EQ(AK_Json_Parse(Context, InvalidStr2), NULL);
    ASSERT_EQ(AK_Json_Get_Error_Code(), AK_JSON_ERROR_CODE_UNDEFINED);
    
    ASSERT_EQ(strcmp(AK_Json_Get_Error_Message().Str, ErrorMessage.Str), 0);
    
    AK_Json_Delete(Context);
}
#endif

UTEST_MAIN();
#ifndef AK_JSON_H
#define AK_JSON_H

#ifdef AK_JSON_STATIC
#define AK_JSON_DEF static
#else
#ifdef __cplusplus
#define AK_JSON_DEF extern "C"
#else
#define AK_JSON_DEF extern
#endif
#endif

typedef unsigned char ak_json_u8;
typedef unsigned long long ak_json_u64;
typedef ak_json_u64 ak_json_user_data;

typedef enum ak_json_error_code
{
    AK_JSON_ERROR_CODE_NONE,
    AK_JSON_ERROR_CODE_OUT_OF_MEMORY,
    AK_JSON_ERROR_CODE_UNDEFINED_TOKEN,
    AK_JSON_ERROR_CODE_EXPECTED_END_OF_STREAM
} ak_json_error_code;

typedef enum ak_json_value_type
{
    AK_JSON_VALUE_TYPE_NULL,
    AK_JSON_VALUE_TYPE_BOOLEAN,
    AK_JSON_VALUE_TYPE_NUMBER,
    AK_JSON_VALUE_TYPE_STRING,
    AK_JSON_VALUE_TYPE_ARRAY,
    AK_JSON_VALUE_TYPE_OBJECT
} ak_json_value_type;

typedef struct ak_json_str
{
    const ak_json_u8* Str;
    ak_json_u64       Length;
} ak_json_str;

#define AK_Json_Str(str) AK_Json_Str_Create((const ak_json_u8*)str, sizeof(str)-1)

typedef struct ak_json_allocator ak_json_allocator;
typedef struct ak_json_context   ak_json_context;
typedef struct ak_json_key       ak_json_key;
typedef struct ak_json_value     ak_json_value;
typedef struct ak_json_array     ak_json_array;
typedef struct ak_json_object    ak_json_object;

typedef void* ak_json_alloc(ak_json_allocator* Allocator, unsigned int Size);
typedef void  ak_json_free(ak_json_allocator* Allocator, void* Memory);

typedef struct ak_json_allocator
{
    ak_json_alloc*    Allocate;
    ak_json_free*     Free;
    ak_json_user_data UserData;
} ak_json_allocator;

AK_JSON_DEF ak_json_str AK_Json_Str_Create(const ak_json_u8* Str, ak_json_u64 Length);

AK_JSON_DEF ak_json_context*   AK_Json_Create(ak_json_allocator* Allocator);
AK_JSON_DEF void               AK_Json_Delete(ak_json_context* Context);

AK_JSON_DEF ak_json_error_code AK_Json_Get_Error_Code();
AK_JSON_DEF ak_json_str        AK_Json_Get_Error_Message();

AK_JSON_DEF ak_json_value* AK_Json_Parse(ak_json_context* Context, ak_json_str Str);

AK_JSON_DEF ak_json_str    AK_Json_Key_Get_Name(ak_json_key* Key);
AK_JSON_DEF ak_json_value* AK_Json_Key_Get_Value(ak_json_key* Key);

AK_JSON_DEF ak_json_value_type AK_Json_Value_Get_Type(ak_json_value* Value);
AK_JSON_DEF ak_json_str        AK_Json_Value_Get_String(ak_json_value* Value);
AK_JSON_DEF int                AK_Json_Value_Get_Boolean(ak_json_value* Value);
AK_JSON_DEF double             AK_Json_Value_Get_Number(ak_json_value* Value);
AK_JSON_DEF ak_json_array*     AK_Json_Value_Get_Array(ak_json_value* Value);
AK_JSON_DEF ak_json_object*    AK_Json_Value_Get_Object(ak_json_value* Value);

AK_JSON_DEF unsigned int     AK_Json_Array_Get_Length(ak_json_array* Array);
AK_JSON_DEF ak_json_value*   AK_Json_Array_Get_Value(ak_json_array* Array, unsigned int Index);

AK_JSON_DEF unsigned int AK_Json_Object_Get_Key_Count(ak_json_object* Object);
AK_JSON_DEF ak_json_key* AK_Json_Object_Get_Key_By_Index(ak_json_object* Object, unsigned int Index);
AK_JSON_DEF ak_json_key* AK_Json_Object_Get_Key(ak_json_object* Object, ak_json_str Key);

#endif

#ifdef AK_JSON_IMPLEMENTATION

#ifndef AK_JSON_ASSERT
#include <assert.h>
#define AK_JSON_ASSERT(cond) assert(cond)
#endif

#if !defined(AK_JSON_MEMSET) || !defined(AK_JSON_MEMCPY)
#include <string.h>
#endif

#ifndef AK_JSON_MEMSET
#define AK_JSON_MEMSET(a,b,c) memset(a,b,c)
#endif

#ifndef AK_JSON_MEMCPY
#define AK_JSON_MEMCPY(a,b,c) memcpy(a,b,c)
#endif

#if !defined(AK_JSON_MALLOC) || !defined(AK_JSON_FREE) || !defined(AK_JSON_ATOF)
#include <stdlib.h>
#endif

#if !defined(AK_JSON_MALLOC) || !defined(AK_JSON_FREE)
#undef AK_JSON_MALLOC
#undef AK_JSON_FREE

#define AK_JSON_MALLOC(size) malloc(size)
#define AK_JSON_FREE(memory) free(memory)
#endif

#if !defined(AK_JSON_ATOF)
#define AK_JSON_ATOF(str) atof(str)
#endif

#if !defined(AK_JSON_SNPRINTF)
#include <stdio.h>
#define AK_JSON_SNPRINTF(a,b,c,...) snprintf(a,b,c,__VA_ARGS__)
#endif

/*************
*** Common ***
**************/

#define AK_JSON__INTERNAL_ERROR_OUT_OF_MEMORY AK_Json_Str("Out of memory")
#define AK_JSON__INTERNAL_ERROR_EXPECTED_EOF AK_Json_Str("Expected EOF")

#define STB_SPRINTF_STATIC

static void AK_Json__Set_Error(ak_json_error_code Code, ak_json_str Message);

static unsigned int AK_Json__Max(unsigned int a, unsigned int b)
{
    return a > b ? a : b;
}

static void AK_Json__Memory_Clear(void* Memory, unsigned int Size)
{
    AK_JSON_MEMSET(Memory, 0, Size);
}

static void AK_Json__Memory_Copy(void* Dst, const void* Src, unsigned int Length)
{
    AK_JSON_MEMCPY(Dst, Src, Length);
}

static void* AK_Json__Default_Allocate(ak_json_allocator* Allocator, unsigned int Size)
{
    return AK_JSON_MALLOC(Size);
}

static void AK_Json__Default_Free(ak_json_allocator* Allocator, void* Memory)
{
    return AK_JSON_FREE(Memory);
}

static ak_json_allocator AK_Json__Get_Default_Allocator()
{
    ak_json_allocator Allocator;
    Allocator.Allocate = AK_Json__Default_Allocate;
    Allocator.Free = AK_Json__Default_Free;
    Allocator.UserData = 0;
    return Allocator;
}

static void* AK_Json__Allocate(ak_json_allocator* Allocator, unsigned int Size)
{
    void* Memory = Allocator->Allocate(Allocator, Size);
    if(!Memory)
    {
        AK_Json__Set_Error(AK_JSON_ERROR_CODE_OUT_OF_MEMORY, AK_JSON__INTERNAL_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    return Memory;
}

static void AK_Json__Free(ak_json_allocator* Allocator, void* Memory)
{
    if(Memory) Allocator->Free(Allocator, Memory);
}

static int AK_Json__Is_Whitespace_Char(ak_json_u8 C)
{
    return C == ' ' || C == '\n' || C == '\t' || C == '\f' || C == '\v';
}

static int AK_Json__Is_Digit(ak_json_u8 C)
{
    return C >= '0' && C <= '9';
}

static int AK_Json__Is_Hex_Digit(ak_json_u8 C)
{
    return AK_Json__Is_Digit(C) || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F');
}

static ak_json_u64 AK_Json__Copy_Whitespace(ak_json_u8* Buffer, ak_json_str Line)
{
    ak_json_u64 Index;
    for(Index = 0; Index < Line.Length; Index++)
    {
        if(AK_Json__Is_Whitespace_Char(Line.Str[Index]))
            Buffer[Index] = Line.Str[Index];
        else
            return Index;
    }
    //NOTE(EVERYONE): This shouldn't happen
    AK_JSON_ASSERT(0);
    return (ak_json_u64)-1;
}


/************
*** Arena ***
*************/

typedef struct ak_json__arena_block
{
    ak_json_u8* Memory;
    ak_json_u64 Used;
    ak_json_u64 Size;
    struct ak_json__arena_block* Next;
} ak_json__arena_block;

typedef struct ak_json__arena
{
    ak_json_allocator     Allocator;
    ak_json__arena_block* FirstBlock;
    ak_json__arena_block* LastBlock;
    ak_json__arena_block* CurrentBlock;
    unsigned int          InitialBlockSize;
} ak_json__arena;

typedef struct ak_json__arena_reserve
{
    ak_json__arena* Arena;
    ak_json__arena_block* Block;
    ak_json_u64     Used;
    ak_json_u64     Size;
} ak_json__arena_reserve;

static ak_json__arena* AK_Json__Arena_Create(ak_json_allocator Allocator, unsigned int InitialBlockSize)
{
    unsigned int AllocationSize = InitialBlockSize+sizeof(ak_json__arena)+sizeof(ak_json__arena_block);
    ak_json__arena* Arena = (ak_json__arena*)AK_Json__Allocate(&Allocator, AllocationSize);
    if(!Arena)  return NULL;
    
    Arena->Allocator = Allocator;
    Arena->InitialBlockSize = InitialBlockSize;
    Arena->FirstBlock = Arena->LastBlock = Arena->CurrentBlock = (ak_json__arena_block*)(Arena+1);
    Arena->CurrentBlock->Memory = (ak_json_u8*)(Arena->CurrentBlock+1);
    Arena->CurrentBlock->Used = 0;
    Arena->CurrentBlock->Size = InitialBlockSize;
    Arena->CurrentBlock->Next = NULL;
    
    return Arena;
}

static void AK_Json__Arena_Delete(ak_json__arena* Arena)
{
    if(Arena && Arena->FirstBlock)
    {
        ak_json_allocator Allocator = Arena->Allocator;
        
        ak_json__arena_block* Block;
        for(Block = Arena->FirstBlock->Next; Block;)
        {
            ak_json__arena_block* CurrentBlock = Block;
            Block = Block->Next;
            AK_Json__Free(&Allocator, CurrentBlock);
        }
        AK_Json__Free(&Allocator, Arena);
    }
}

static ak_json__arena_block* AK_Json__Arena_Create_Block(ak_json__arena* Arena, unsigned int BlockSize)
{
    ak_json_allocator Allocator = Arena->Allocator;
    ak_json__arena_block* Block = (ak_json__arena_block*)AK_Json__Allocate(&Allocator, BlockSize+sizeof(ak_json__arena_block));
    if(!Block) return NULL;
    
    Block->Memory = (ak_json_u8*)(Block+1);
    Block->Used = 0;
    Block->Size = BlockSize;
    Block->Next = NULL;
    return Block;
}

static ak_json__arena_block* AK_Json__Arena_Get_Block(ak_json__arena* Arena, unsigned int Size)
{
    ak_json__arena_block* Block = Arena->CurrentBlock;
    if(!Block) return NULL;
    
    while(Block->Used+Size > Block->Size)
    {
        Block = Block->Next;
        if(!Block) return NULL;
    }
    
    return Block;
}

static void AK_Json__Arena_Add_Block(ak_json__arena* Arena, ak_json__arena_block* Block)
{
    ak_json__arena_block* Last = Arena->LastBlock;
    if(!Last)
    {
        AK_JSON_ASSERT(!Arena->FirstBlock);
        Arena->FirstBlock = Arena->LastBlock = Block;
    }
    else
    {
        Last->Next = Block;
        Arena->LastBlock = Block;
    }
}

static void* AK_Json__Arena_Push(ak_json__arena* Arena, unsigned int Size)
{
    if(!Size) return NULL;
    
    ak_json__arena_block* Block = AK_Json__Arena_Get_Block(Arena, Size);
    if(!Block)
    {
        unsigned int BlockSize = AK_Json__Max(Arena->InitialBlockSize, Size);
        Block = AK_Json__Arena_Create_Block(Arena, BlockSize);
        if(!Block) return NULL;
        AK_Json__Arena_Add_Block(Arena, Block);
    }
    
    Arena->CurrentBlock = Block;
    AK_JSON_ASSERT(Arena->CurrentBlock->Used+Size <= Arena->CurrentBlock->Size);
    
    void* Result = Arena->CurrentBlock->Memory + Arena->CurrentBlock->Used;
    Arena->CurrentBlock->Used += Size;
    
    return Result;
}

static ak_json__arena_reserve AK_Json__Arena_Begin_Reserve(ak_json__arena* Arena, unsigned int Size)
{
    ak_json__arena_reserve Reserve;
    Reserve.Size = 0;
    Reserve.Arena  = NULL;
    if(!Size) return Reserve;
    
    ak_json__arena_block* Block = AK_Json__Arena_Get_Block(Arena, Size);
    if(!Block)
    {
        unsigned int BlockSize = AK_Json__Max(Arena->InitialBlockSize, Size);
        Block = AK_Json__Arena_Create_Block(Arena, BlockSize);
        if(!Block) return Reserve;
        AK_Json__Arena_Add_Block(Arena, Block);
    }
    
    Arena->CurrentBlock = Block;
    AK_JSON_ASSERT(Arena->CurrentBlock->Used+Size <= Arena->CurrentBlock->Size);
    
    Reserve.Size = Size;
    Reserve.Arena  = Arena;
    Reserve.Block = Arena->CurrentBlock;
    Reserve.Used  = 0;
    
    return Reserve;
}

static void* AK_Json__Arena_Push_Reserve(ak_json__arena_reserve* Reserve, unsigned int Size)
{
    AK_JSON_ASSERT(Reserve->Block->Used+Reserve->Used+Size <= Reserve->Block->Size);
    void* Result = Reserve->Block->Memory + Reserve->Block->Used + Reserve->Used;
    Reserve->Used += Size;
    return Result;
}

static void AK_Json__Arena_End_Reserve(ak_json__arena* Arena, ak_json__arena_reserve* Reserve)
{
    Reserve->Block->Used += Reserve->Used;
}

static ak_json_u8* AK_Json__Arena_Reserve_Get_Memory(ak_json__arena_reserve* Reserve)
{
    ak_json_u8* Result = Reserve->Block->Memory + Reserve->Block->Used;
    return Result;
}

/**************
*** Strings ***
***************/

#define AK_JSON__BIT_8  0x00000080
#define AK_JSON__BITMASK_2  0x00000003
#define AK_JSON__BITMASK_3  0x00000007
#define AK_JSON__BITMASK_4  0x0000000f
#define AK_JSON__BITMASK_5  0x0000001f
#define AK_JSON__BITMASK_6  0x0000003f

AK_JSON_DEF ak_json_str AK_Json_Str_Create(const ak_json_u8* Str, ak_json_u64 Length)
{ 
    ak_json_str Result = {Str, Length};
    return Result;
}

static ak_json_str AK_Json_Str__Copy(ak_json__arena* Arena, ak_json_str Str)
{
    ak_json_str Result;
    Result.Length = Str.Length;
    char* Buffer = (char*)AK_Json__Arena_Push(Arena, Str.Length+1);
    
    Buffer[Str.Length] = 0;
    AK_Json__Memory_Copy(Buffer, Str.Str, Str.Length);
    
    Result.Str = (const ak_json_u8*)Buffer;
    return Result;
}

static ak_json_str AK_Json_Str__Substr(ak_json_str Str, ak_json_u64 StartIndex, ak_json_u64 EndIndex)
{
    AK_JSON_ASSERT(StartIndex < Str.Length);
    AK_JSON_ASSERT(EndIndex <= Str.Length);
    AK_JSON_ASSERT(StartIndex < EndIndex);
    
    ak_json_str Result;
    Result.Str = Str.Str;
    Result.Length = EndIndex-StartIndex;
    return Result;
}

static int AK_Json_Str__Equal(ak_json_str StrA, ak_json_str StrB)
{
    if(StrA.Length != StrB.Length) return 0;
    
    ak_json_u64 i;
    for(i = 0; i < StrA.Length; i++)
    {
        if(StrA.Str[i] != StrB.Str[i]) 
            return 0;
    }
    return 1;
}

unsigned int AK_Json__Get_UTF8_Bytecount(unsigned int Codepoint)
{
    unsigned int Result = 0;
    if (Codepoint <= 0x7F)
    {
        Result = 1;
    }
    else if (Codepoint <= 0x7FF)
    {
        Result = 2;
    }
    else if (Codepoint <= 0xFFFF)
    {
        Result = 3;
    }
    else if (Codepoint <= 0x10FFFF)
    {
        Result = 4;
    }
    else
    {
        AK_JSON_ASSERT(0);
    }
    return Result;
}

void AK_Json__UTF8_From_Codepoint(unsigned int Codepoint, ak_json_u8* Buffer)
{
    unsigned int Offset = 0;
    if (Codepoint <= 0x7F)
    {
        Buffer[0] = (ak_json_u8)Codepoint;
        Offset = 1;
    }
    else if (Codepoint <= 0x7FF)
    {
        Buffer[0] = (AK_JSON__BITMASK_2 << 6) | ((Codepoint >> 6) & AK_JSON__BITMASK_5);
        Buffer[1] = AK_JSON__BIT_8 | (Codepoint & AK_JSON__BITMASK_6);
        Offset = 2;
    }
    else if (Codepoint <= 0xFFFF)
    {
        Buffer[0] = (AK_JSON__BITMASK_3 << 5) | ((Codepoint >> 12) & AK_JSON__BITMASK_4);
        Buffer[1] = AK_JSON__BIT_8 | ((Codepoint >> 6) & AK_JSON__BITMASK_6);
        Buffer[2] = AK_JSON__BIT_8 | ( Codepoint       & AK_JSON__BITMASK_6);
        Offset = 3;
    }
    else if (Codepoint <= 0x10FFFF)
    {
        Buffer[0] = (AK_JSON__BITMASK_4 << 3) | ((Codepoint >> 18) & AK_JSON__BITMASK_3);
        Buffer[1] = AK_JSON__BIT_8 | ((Codepoint >> 12) & AK_JSON__BITMASK_6);
        Buffer[2] = AK_JSON__BIT_8 | ((Codepoint >>  6) & AK_JSON__BITMASK_6);
        Buffer[3] = AK_JSON__BIT_8 | ( Codepoint        & AK_JSON__BITMASK_6);
        Offset = 4;
    }
    else
    {
        AK_JSON_ASSERT(0);
    }
}

ak_json_str AK_Json__Json_Str_To_UTF8(ak_json__arena* Arena, ak_json_str JsonStr)
{
    ak_json__arena_reserve Reserve = AK_Json__Arena_Begin_Reserve(Arena, JsonStr.Length+1);
    
    ak_json_u64 Index;
    for(Index = 0; Index < JsonStr.Length; Index++)
    {
        if(JsonStr.Str[Index] == '\\')
        {
            if(JsonStr.Str[Index+1] == 'u')
            {
                unsigned int Codepoint = ((JsonStr.Str[Index+5] << 24) | 
                                          (JsonStr.Str[Index+4] << 16) | 
                                          (JsonStr.Str[Index+3] << 8)  | 
                                          (JsonStr.Str[Index+2] << 0));
                unsigned int ByteCount = AK_Json__Get_UTF8_Bytecount(Codepoint);
                ak_json_u8* Memory = AK_Json__Arena_Push_Reserve(&Reserve, ByteCount);
                AK_Json__UTF8_From_Codepoint(Codepoint, Memory);
                Index += 5;
                continue;
            }
        }
        
        ak_json_u8* Memory = AK_Json__Arena_Push_Reserve(&Reserve, 1);
        *Memory = JsonStr.Str[Index];
    }
    
    ak_json_str Result;
    Result.Length = Reserve.Used;
    
    ak_json_u8* Memory = AK_Json__Arena_Reserve_Get_Memory(&Reserve);
    
    Memory[Reserve.Used] = 0;
    Result.Str = (const ak_json_u8*)Memory;
    
    AK_Json__Arena_End_Reserve(Arena, &Reserve);
    
    return Result;
}

/************************
*** Creating/Deleting ***
*************************/

typedef struct ak_json_context
{
    ak_json__arena* Arena;
    ak_json_key*    FreeKeys;
    ak_json_value*  FreeValues;
} ak_json_context;

AK_JSON_DEF ak_json_context* AK_Json_Create(ak_json_allocator* pAllocator)
{
    ak_json_allocator Allocator = pAllocator ? *pAllocator : AK_Json__Get_Default_Allocator();
    ak_json__arena* Arena = AK_Json__Arena_Create(Allocator, 1024*1024);
    if(!Arena) return NULL;
    
    ak_json_context* Result = (ak_json_context*)AK_Json__Arena_Push(Arena, sizeof(ak_json_context));
    Result->Arena = Arena;
    return Result;
}

AK_JSON_DEF void AK_Json_Delete(ak_json_context* Context)
{
    if(Context)
    {
        ak_json__arena* Arena = Context->Arena;
        AK_Json__Arena_Delete(Arena);
    }
}

/*************
*** Errors ***
**************/

typedef struct ak_json__error
{
    ak_json_error_code Code;
    ak_json_str        Message;
} ak_json__error;

static ak_json__error G_AK_Json__Internal_Error;

static void AK_Json__Set_Error(ak_json_error_code Code, ak_json_str Message)
{
    G_AK_Json__Internal_Error.Code    = Code;
    G_AK_Json__Internal_Error.Message = Message;
}

AK_JSON_DEF ak_json_error_code AK_Json_Get_Error_Code()
{
    return G_AK_Json__Internal_Error.Code;
}

AK_JSON_DEF ak_json_str AK_Json_Get_Error_Message()
{
    return G_AK_Json__Internal_Error.Message;
}

/**************
*** Parsing ***
***************/

typedef struct ak_json__line
{
    ak_json_u64 Number;
    ak_json_u64 StartIndex;
    ak_json_u64 EndIndex;
} ak_json__line;

static ak_json__line AK_Json__Line_Null()
{
    ak_json__line Line;
    Line.Number = 0;
    Line.StartIndex = (ak_json_u64)-1;
    Line.EndIndex = (ak_json_u64)-1;
    return Line;
}

static ak_json_str AK_Json__Line_Get_Str(ak_json_str Str, ak_json__line Line)
{
    return AK_Json_Str__Substr(Str, Line.StartIndex, Line.EndIndex);
}

typedef struct ak_json__char
{
    ak_json__line PreviousLine;
    ak_json__line CurrentLine;
    ak_json_u64 Index;
    ak_json_u8  Char;
} ak_json__char;

static void AK_Json__Error_Log(ak_json__arena* Arena, ak_json_str Str, ak_json_error_code ErrorCode, ak_json__char Char, ak_json_str Message)
{
    ak_json_u64 LineIndex = Char.CurrentLine.Number;
    static char TempBuffer[1];
    unsigned int Length;
    const char* Format;
    ak_json_str PreviousLineStr;
    ak_json_str CurrentLineStr;
    
    if(LineIndex > 1)
    {
        Format = "Error: %.*s\n%d %.*s\n%d %.*s\n";
        PreviousLineStr = AK_Json__Line_Get_Str(Str, Char.PreviousLine);
        CurrentLineStr = AK_Json__Line_Get_Str(Str, Char.CurrentLine);
        Length = AK_JSON_SNPRINTF(TempBuffer, 1, Format, Message.Length, Message.Str, Char.PreviousLine.Number, 
                                  PreviousLineStr.Length, PreviousLineStr.Str, Char.CurrentLine.Number, CurrentLineStr.Length, CurrentLineStr.Str);
    }
    else
    {
        Format = "Error: %.*s\n%d %.*s\n";
        CurrentLineStr = AK_Json__Line_Get_Str(Str, Char.CurrentLine);
        Length = AK_JSON_SNPRINTF(TempBuffer, 1, Format, Message.Length, Message.Str, Char.CurrentLine.Number, 
                                  CurrentLineStr.Length, CurrentLineStr.Str);
    }
    
    //NOTE(EVERYONE): The plus 2 is the offset of line number plus space, and then the final character to add 
    unsigned int CharacterCount = Length+2+(Char.Index-Char.CurrentLine.StartIndex)+1;
    char* Buffer = (char*)AK_Json__Arena_Push(Arena, CharacterCount+1);
    Buffer[CharacterCount] = 0;
    
    if(LineIndex > 1)
    {
        AK_JSON_SNPRINTF(Buffer, CharacterCount, Format, Message.Length, Message.Str, Char.PreviousLine.Number, PreviousLineStr.Length, PreviousLineStr.Str, Char.CurrentLine.Number, CurrentLineStr.Length, CurrentLineStr.Str);
    }
    else
    {
        AK_JSON_SNPRINTF(Buffer, CharacterCount, Format, Message.Length, Message.Str, Char.CurrentLine.Number, 
                         CurrentLineStr.Length, CurrentLineStr.Str);
    }
    
    
    char* FinalLine = Buffer + Length;
    *FinalLine++ = ' ';
    *FinalLine++ = ' ';
    
    unsigned int WhitespaceCount = AK_Json__Copy_Whitespace((ak_json_u8*)FinalLine, CurrentLineStr);
    FinalLine[WhitespaceCount] = '^';
    
    ak_json_str Result;
    Result.Str = (const ak_json_u8*)Buffer;
    Result.Length = CharacterCount;
    
    AK_Json__Set_Error(ErrorCode, Result);
}

typedef struct ak_json__stream
{
    ak_json_str   Str;
    ak_json_u64   StrIndex;
    ak_json_u64   LineNumber;
    ak_json__line PreviousLine;
    ak_json__line CurrentLine;
} ak_json__stream;

static void AK_Json__Stream_Consume_Line(ak_json__stream* Stream)
{
    ak_json_u64 Index = Stream->StrIndex;
    while(Index < Stream->Str.Length)
    {
        if(Stream->Str.Str[Index] == '\r')
        {
            if(!Index || Stream->Str.Str[Index-1] != '\n')
                break;
        }
        else if(Stream->Str.Str[Index] == '\n')
        {
            if(!Index || Stream->Str.Str[Index-1] != '\r')
                break;
        }
        
        Index++;
    }
    
    ak_json__line Line;
    Line.StartIndex = Stream->StrIndex;
    Line.EndIndex = Index;
    Line.Number = Stream->LineNumber++;
    
    Stream->PreviousLine = Stream->CurrentLine;
    Stream->CurrentLine = Line;
}

static int AK_Json__Stream_Is_Valid(ak_json__stream* Stream)
{
    return Stream->StrIndex < Stream->Str.Length;
}

static ak_json_u64 AK_Json__Stream_Get_Remaining(ak_json__stream* Stream)
{
    AK_JSON_ASSERT(Stream->StrIndex <= Stream->Str.Length);
    return Stream->Str.Length-Stream->StrIndex;
}

static void AK_Json__Stream_Increment(ak_json__stream* Stream)
{
    AK_JSON_ASSERT(Stream->StrIndex < Stream->Str.Length);
    AK_JSON_ASSERT(Stream->StrIndex < Stream->CurrentLine.EndIndex);
    Stream->StrIndex++;
    if(Stream->CurrentLine.EndIndex == Stream->StrIndex) AK_Json__Stream_Consume_Line(Stream);
}

static ak_json__char AK_Json__Stream_Peek_Char(ak_json__stream* Stream)
{
    AK_JSON_ASSERT(Stream->StrIndex < Stream->Str.Length);
    AK_JSON_ASSERT(Stream->StrIndex < Stream->CurrentLine.EndIndex);
    
    ak_json__char Result;
    Result.PreviousLine = Stream->PreviousLine;
    Result.CurrentLine = Stream->CurrentLine;
    Result.Index = Stream->StrIndex;
    Result.Char = Stream->Str.Str[Stream->StrIndex];
    return Result;
}

static ak_json__char AK_Json__Stream_Consume_Char(ak_json__stream* Stream)
{
    ak_json__char Result = AK_Json__Stream_Peek_Char(Stream);
    AK_Json__Stream_Increment(Stream);
    return Result;
}

static void AK_Json__Stream_Eat_Whitespace(ak_json__stream* Stream)
{
    ak_json_str* Str = &Stream->Str;
    while(AK_Json__Stream_Is_Valid(Stream))
    {
        ak_json__char Char = AK_Json__Stream_Peek_Char(Stream);
        if(!AK_Json__Is_Whitespace_Char(Char.Char))
            break;
        AK_Json__Stream_Increment(Stream);
    }
}

static void AK_Json__Stream_Eat_Digits(ak_json__stream* Stream)
{
    ak_json_str* Str = &Stream->Str;
    while(AK_Json__Stream_Is_Valid(Stream))
    {
        ak_json__char Char = AK_Json__Stream_Peek_Char(Stream);
        if(!AK_Json__Is_Digit(Char.Char))
            break;
        AK_Json__Stream_Increment(Stream);
    }
}

static ak_json__stream AK_Json__Stream_Create(ak_json_str Str)
{
    ak_json__stream Stream;
    Stream.Str           = Str;
    Stream.LineNumber    = 1;
    Stream.StrIndex      = 0;
    Stream.PreviousLine  = AK_Json__Line_Null();
    Stream.CurrentLine   = AK_Json__Line_Null();
    AK_Json__Stream_Consume_Line(&Stream);
    return Stream;
}

typedef enum ak_json__token_type
{
    AK_JSON__TOKEN_TYPE_UNDEFINED,
    AK_JSON__TOKEN_TYPE_NULL,
    AK_JSON__TOKEN_TYPE_BOOLEAN,
    AK_JSON__TOKEN_TYPE_NUMBER,
    AK_JSON__TOKEN_TYPE_STRING
} ak_json__token_type;

typedef struct ak_json__token
{
    ak_json__token_type    Type;
    ak_json__char          StartChar;
    ak_json_u64            Length;
    struct ak_json__token* Next;
} ak_json__token;

static ak_json_str AK_Json__Token_Get_Str(ak_json_str Str, ak_json__token* Token)
{
    ak_json_str Result = AK_Json_Str__Substr(Str, Token->StartChar.Index, Token->StartChar.Index+Token->Length);
    return Result;
}

typedef struct ak_json__token_list
{
    ak_json__token* First;
    ak_json__token* Last;
    ak_json_u64     Count;
} ak_json__token_list;

static void AK_Json__Token_List_Add(ak_json__token_list* List, ak_json__token* Token)
{
    if(!List->First) List->First = Token;
    else List->Last->Next = Token;
    List->Last = Token;
    List->Count++;
}

typedef struct ak_json__tokenizer
{
    ak_json__arena*     TokenArena;
    ak_json__arena*     ErrorArena;
    ak_json__token_list Tokens;
} ak_json__tokenizer;

static ak_json__token* AK_Json__Tokenizer_Allocate_Token(ak_json__tokenizer* Tokenizer, ak_json__token_type Type, ak_json__char StartChar)
{
    ak_json__token* Token = (ak_json__token*)AK_Json__Arena_Push(Tokenizer->TokenArena, sizeof(ak_json__token));
    Token->Type = Type;
    Token->StartChar = StartChar;
    Token->Length = (ak_json_u64)-1;
    Token->Next = NULL;
    
    AK_Json__Token_List_Add(&Tokenizer->Tokens, Token);
    return Token;
}

#define AK_Json__Return_Undefined(Char) \
ak_json__token* Token = AK_Json__Tokenizer_Allocate_Token(Tokenizer, AK_JSON__TOKEN_TYPE_UNDEFINED, Char); \
return 0

static int AK_Json__Tokenize_Null(ak_json__tokenizer* Tokenizer, ak_json__stream* Stream)
{
    ak_json__char Char1 = AK_Json__Stream_Consume_Char(Stream);
    AK_JSON_ASSERT(Char1.Char == 'n');
    
    ak_json_u64 Remaining = AK_Json__Stream_Get_Remaining(Stream);
    if(Remaining >= 3)
    {
        ak_json__char Char2 = AK_Json__Stream_Consume_Char(Stream);
        ak_json__char Char3 = AK_Json__Stream_Consume_Char(Stream);
        ak_json__char Char4 = AK_Json__Stream_Consume_Char(Stream);
        
        if(Char2.Char == 'u' && Char3.Char == 'l' && Char4.Char == 'l')
        {
            ak_json__token* Token = AK_Json__Tokenizer_Allocate_Token(Tokenizer, AK_JSON__TOKEN_TYPE_NULL, Char1);
            Token->Length = 4;
            return 1;
        }
    }
    
    AK_Json__Return_Undefined(Char1);
}

static int AK_Json__Tokenize_Boolean(ak_json__tokenizer* Tokenizer, ak_json__stream* Stream)
{
    ak_json__char Char1 = AK_Json__Stream_Consume_Char(Stream);
    AK_JSON_ASSERT(Char1.Char == 't' || Char1.Char == 'f');
    
    ak_json_u64 Remaining = AK_Json__Stream_Get_Remaining(Stream);
    ak_json_u64 Target = Char1.Char == 't' ? 4 : 5;
    
    ak_json_u8 Chars[5] = {Char1.Char};
    if(Remaining >= (Target-1))
    {
        ak_json_u64 Index;
        for(Index = 1; Index < Target; Index++)
            Chars[Index] = AK_Json__Stream_Consume_Char(Stream).Char;
        
        if(Chars[0] == 't' && Chars[1] == 'r' && Chars[2] == 'u' && Chars[3] == 'e')
        {
            ak_json__token* Token = AK_Json__Tokenizer_Allocate_Token(Tokenizer, AK_JSON__TOKEN_TYPE_BOOLEAN, Char1);
            Token->Length = 4;
            return 1;
        }
        else if(Chars[0] == 'f' && Chars[1] == 'a' && Chars[2] == 'l' && Chars[3] == 's' && Chars[4] == 'e')
        {
            ak_json__token* Token = AK_Json__Tokenizer_Allocate_Token(Tokenizer, AK_JSON__TOKEN_TYPE_BOOLEAN, Char1);
            Token->Length = 5;
            return 1;
        }
    }
    
    AK_Json__Return_Undefined(Char1);
}

static int AK_Json__Tokenize_Number(ak_json__tokenizer* Tokenizer, ak_json__stream* Stream)
{
    ak_json__char StartChar = AK_Json__Stream_Consume_Char(Stream);
    
    if(AK_Json__Is_Digit(StartChar.Char) || StartChar.Char == '-')
    {
        ak_json__char Char = StartChar;
        if(Char.Char == '-') 
        {
            if(!AK_Json__Stream_Is_Valid(Stream))
            {
                AK_Json__Return_Undefined(StartChar);
            }
            
            Char = AK_Json__Stream_Consume_Char(Stream);
        }
        
        int IsZero = Char.Char == '0';
        if(!IsZero) AK_Json__Stream_Eat_Digits(Stream);
        if(AK_Json__Stream_Is_Valid(Stream))
        {
            ak_json__char Char = AK_Json__Stream_Peek_Char(Stream);
            if(IsZero && AK_Json__Is_Digit(Char.Char))
            {
                AK_Json__Return_Undefined(StartChar);
            }
            
            if(Char.Char == '.') 
            {
                AK_Json__Stream_Increment(Stream);
                if(AK_Json__Stream_Is_Valid(Stream))
                {
                    Char = AK_Json__Stream_Peek_Char(Stream);
                    if(AK_Json__Is_Digit(Char.Char))
                        AK_Json__Stream_Eat_Digits(Stream);
                    else
                    {
                        AK_Json__Return_Undefined(StartChar);
                    }
                }
                else
                {
                    AK_Json__Return_Undefined(StartChar);
                }
            }
            
            if(AK_Json__Stream_Is_Valid(Stream))
            {
                Char = AK_Json__Stream_Peek_Char(Stream);
                if(Char.Char == 'e' || Char.Char == 'E')
                {
                    AK_Json__Stream_Increment(Stream);
                    if(!AK_Json__Stream_Is_Valid(Stream))
                    {
                        AK_Json__Return_Undefined(StartChar);
                    }
                    
                    Char = AK_Json__Stream_Peek_Char(Stream);
                    if(Char.Char == '-' || Char.Char == '+')
                    {
                        AK_Json__Stream_Increment(Stream);
                        if(!AK_Json__Stream_Is_Valid(Stream))
                        {
                            AK_Json__Return_Undefined(StartChar);
                        }
                        
                        Char = AK_Json__Stream_Peek_Char(Stream);
                    }
                    
                    if(AK_Json__Is_Digit(Char.Char))
                        AK_Json__Stream_Eat_Digits(Stream);
                    else
                    {
                        AK_Json__Return_Undefined(StartChar);
                    }
                }
            }
        }
        
        ak_json__token* Token = AK_Json__Tokenizer_Allocate_Token(Tokenizer, AK_JSON__TOKEN_TYPE_NUMBER, StartChar);
        Token->Length = Stream->StrIndex-StartChar.Index;
        return 1;
    }
    else
    {
        AK_Json__Return_Undefined(StartChar);
    }
}

static int AK_Json__Tokenize_String(ak_json__tokenizer* Tokenizer, ak_json__stream* Stream)
{
    ak_json__char StartChar = AK_Json__Stream_Consume_Char(Stream);
    AK_JSON_ASSERT(StartChar.Char == '"');
    
    while(AK_Json__Stream_Is_Valid(Stream))
    {
        ak_json__char Char = AK_Json__Stream_Consume_Char(Stream);
        
        if(Char.Char == '"')
        {
            ak_json__token* Token = AK_Json__Tokenizer_Allocate_Token(Tokenizer, AK_JSON__TOKEN_TYPE_STRING, StartChar);
            Token->Length = Stream->StrIndex-StartChar.Index;
            return 1;
        }
        else if(Char.Char == '\\')
        {
            if(AK_Json__Stream_Is_Valid(Stream))
            {
                ak_json__char ControlChar = AK_Json__Stream_Consume_Char(Stream);
                switch(ControlChar.Char)
                {
                    case '"':
                    case '\\':
                    case '/':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                    {
                        //NOTE(EVERYONE): Noop. These are the valid control points
                    } break;
                    
                    case 'u':
                    {
                        ak_json_u64 Remaining = AK_Json__Stream_Get_Remaining(Stream);
                        if(Remaining >= 4)
                        {
                            ak_json_u64 Index;
                            for(Index = 0; Index < 4; Index++)
                            {
                                ak_json__char UnicodeChar = AK_Json__Stream_Consume_Char(Stream);
                                if(!AK_Json__Is_Hex_Digit(UnicodeChar.Char))
                                    AK_Json__Return_Undefined(StartChar);
                            }
                        }
                        else
                        {
                            AK_Json__Return_Undefined(StartChar);
                        }
                    } break;
                    
                    default:
                    {
                        AK_Json__Return_Undefined(StartChar);
                    } break;
                }
            }
            else
            {
                AK_Json__Return_Undefined(StartChar);
            }
        }
    }
    
    AK_Json__Return_Undefined(StartChar);
}

#undef AK_Json__Return_Undefined

static int AK_Json__Tokenize_Value(ak_json__tokenizer* Tokenizer, ak_json__stream* Stream)
{
    AK_Json__Stream_Eat_Whitespace(Stream);
    
    ak_json__char Char = AK_Json__Stream_Peek_Char(Stream);
    
    int Result = 1;
    switch(Char.Char)
    {
        case 'n':
        {
            Result = AK_Json__Tokenize_Null(Tokenizer, Stream);
            if(!Result)
            {
                AK_Json__Error_Log(Tokenizer->ErrorArena, Stream->Str, AK_JSON_ERROR_CODE_UNDEFINED_TOKEN, Char, AK_Json_Str("Expecting null value. Got undefined."));
            }
        } break;
        
        case 't':
        case 'f':
        {
            Result = AK_Json__Tokenize_Boolean(Tokenizer, Stream);
            if(!Result)
            {
                AK_Json__Error_Log(Tokenizer->ErrorArena, Stream->Str, AK_JSON_ERROR_CODE_UNDEFINED_TOKEN, Char, AK_Json_Str("Expecting boolean value. Got undefined."));
            }
        } break;
        
        case '"':
        {
            Result = AK_Json__Tokenize_String(Tokenizer, Stream);
            if(!Result)
            {
                AK_Json__Error_Log(Tokenizer->ErrorArena, Stream->Str, AK_JSON_ERROR_CODE_UNDEFINED_TOKEN, Char, AK_Json_Str("Expecting string value. Got undefined."));
            }
        } break;
        
        default:
        {
            Result = AK_Json__Tokenize_Number(Tokenizer, Stream);
            if(!Result)
            {
                AK_Json__Error_Log(Tokenizer->ErrorArena, Stream->Str, AK_JSON_ERROR_CODE_UNDEFINED_TOKEN, Char, AK_Json_Str("Expecting numeric value. Got undefined."));
            }
        } break;
    }
    
    if(Result) AK_Json__Stream_Eat_Whitespace(Stream);
    return Result;
}

static int AK_Json__Tokenize(ak_json__tokenizer* Tokenizer, ak_json_str Str)
{
    ak_json__stream Stream = AK_Json__Stream_Create(Str);
    AK_Json__Stream_Eat_Whitespace(&Stream);
    
    int Result = 0;
    if(AK_Json__Stream_Is_Valid(&Stream))
    {
        ak_json__char Char = AK_Json__Stream_Peek_Char(&Stream);
        switch(Char.Char)
        {
            case '{':
            {
                //Result = AK_Json__Tokenize_Object(Tokenizer, Stream);
            } break;
            
            case '[':
            {
                //Result = AK_Json__Tokenize_Array(Tokenizer, Stream);
            } break;
            
            default:
            {
                Result = AK_Json__Tokenize_Value(Tokenizer, &Stream);
            } break;
        }
    }
    
    if(Result)
    {
        AK_Json__Stream_Eat_Whitespace(&Stream);
        if(AK_Json__Stream_Is_Valid(&Stream))
        {
            //NOTE(EVERYONE): There shouldn't be anymore remaining items here
            //TODO(JJ): Log error
            ak_json__char Char = AK_Json__Stream_Peek_Char(&Stream);
            AK_Json__Error_Log(Tokenizer->ErrorArena, Str, AK_JSON_ERROR_CODE_EXPECTED_END_OF_STREAM, Char, AK_JSON__INTERNAL_ERROR_EXPECTED_EOF);
            return 0;
        }
    }
    
    return Result;
}

typedef struct ak_json_object
{
} ak_json_object;

typedef struct ak_json_array
{
} ak_json_array;

typedef struct ak_json_value
{
    ak_json_value_type Type;
    union
    {
        ak_json_str    String;
        int            Boolean;
        double         Number;
        ak_json_array  Array;
        ak_json_object Object;
    };
    
    ak_json_value* Next;
} ak_json_value;

typedef struct ak_json_key
{
    ak_json_str    Str;
    ak_json_value* Value;
    ak_json_key*   Next;
} ak_json_key;

static ak_json_value* AK_Json__Parse_Null_Value(ak_json__arena* Arena, ak_json_str Str, ak_json__token* Token)
{
    AK_JSON_ASSERT(Token->Type == AK_JSON__TOKEN_TYPE_NULL);
    ak_json_value* Value = (ak_json_value*)AK_Json__Arena_Push(Arena, sizeof(ak_json_value));
    Value->Type = AK_JSON_VALUE_TYPE_NULL;
    return Value;
}

static ak_json_value* AK_Json__Parse_Boolean_Value(ak_json__arena* Arena, ak_json_str Str, ak_json__token* Token)
{
    AK_JSON_ASSERT(Token->Type == AK_JSON__TOKEN_TYPE_BOOLEAN);
    ak_json_str TokenStr = AK_Json__Token_Get_Str(Str, Token);
    
    int Boolean = AK_Json_Str__Equal(TokenStr, AK_Json_Str("true")) ? 1 : 0;
    ak_json_value* Value = (ak_json_value*)AK_Json__Arena_Push(Arena, sizeof(ak_json_value));
    Value->Type = AK_JSON_VALUE_TYPE_BOOLEAN;
    Value->Boolean = Boolean;
    return Value;
}

static ak_json_value* AK_Json__Parse_Number_Value(ak_json__arena* Arena, ak_json_str Str, ak_json__token* Token)
{
    AK_JSON_ASSERT(Token->Type == AK_JSON__TOKEN_TYPE_NUMBER);
    
    ak_json_str TokenStr = AK_Json__Token_Get_Str(Str, Token);
    
    ak_json_value* Value = (ak_json_value*)AK_Json__Arena_Push(Arena, sizeof(ak_json_value));
    Value->Type = AK_JSON_VALUE_TYPE_NUMBER;
    Value->Number = AK_JSON_ATOF((const char*)TokenStr.Str);
    return Value;
}

static ak_json_value* AK_Json__Parse_String_Value(ak_json__arena* Arena, ak_json_str Str, ak_json__token* Token)
{
    AK_JSON_ASSERT(Token->Type == AK_JSON__TOKEN_TYPE_STRING);
    
    ak_json_str TokenStr = AK_Json__Token_Get_Str(Str, Token);
    
    ak_json_value* Value = (ak_json_value*)AK_Json__Arena_Push(Arena, sizeof(ak_json_value));
    Value->Type = AK_JSON_VALUE_TYPE_STRING;
    Value->String = AK_Json__Json_Str_To_UTF8(Arena, TokenStr);
    
    return Value;
}

static ak_json_value* AK_Json__Value_Copy(ak_json_context* Context, ak_json_value* CopyValue)
{
    ak_json_value* Value = Context->FreeValues;
    if(!Value) Value = (ak_json_value*)AK_Json__Arena_Push(Context->Arena, sizeof(ak_json_value));
    else Context->FreeValues = Context->FreeValues->Next;
    
    Value->Type = CopyValue->Type;
    switch(CopyValue->Type)
    {
        case AK_JSON_VALUE_TYPE_NULL:
        {
            //NOTE(EVERYONE): Do nothing
        } break;
        
        case AK_JSON_VALUE_TYPE_BOOLEAN:
        {
            Value->Boolean = CopyValue->Boolean;
        } break;
        
        case AK_JSON_VALUE_TYPE_NUMBER:
        {
            Value->Number = CopyValue->Number;
        } break;
        
        case AK_JSON_VALUE_TYPE_STRING:
        {
            Value->String = AK_Json_Str__Copy(Context->Arena, CopyValue->String);
        } break;
        
        case AK_JSON_VALUE_TYPE_ARRAY:
        {
            //TODO(JJ): Not implemented
        } break;
        
        case AK_JSON_VALUE_TYPE_OBJECT:
        {
            //TODO(JJ): Not implemented
        } break;
    }
    
    return Value;
}

AK_JSON_DEF ak_json_value* AK_Json_Parse(ak_json_context* Context, ak_json_str Str)
{
    ak_json__arena* ParseArena = AK_Json__Arena_Create(Context->Arena->Allocator, Str.Length*2);
    if(!ParseArena) return NULL;
    
    ak_json__tokenizer Tokenizer;
    AK_Json__Memory_Clear(&Tokenizer, sizeof(ak_json__tokenizer));
    Tokenizer.TokenArena = ParseArena;
    Tokenizer.ErrorArena = Context->Arena;
    
    if(!AK_Json__Tokenize(&Tokenizer, Str)) 
    {
        return NULL;
    }
    
    ak_json__token* Token = Tokenizer.Tokens.First;
    
    ak_json_value* RootValue = NULL;
    switch(Token->Type)
    {
        case AK_JSON__TOKEN_TYPE_NULL:
        {
            RootValue = AK_Json__Parse_Null_Value(ParseArena, Str, Token);
        } break;
        
        case AK_JSON__TOKEN_TYPE_BOOLEAN:
        {
            RootValue = AK_Json__Parse_Boolean_Value(ParseArena, Str, Token);
        } break;
        
        case AK_JSON__TOKEN_TYPE_NUMBER:
        {
            RootValue = AK_Json__Parse_Number_Value(ParseArena, Str, Token);
        } break;
        
        case AK_JSON__TOKEN_TYPE_STRING:
        {
            RootValue = AK_Json__Parse_String_Value(ParseArena, Str, Token);
        } break;
        
        default:
        {
            //NOTE(EVERYONE): This shouldn't occur
            AK_JSON_ASSERT(0);
        } break;
    }
    
    ak_json_value* Result = NULL;
    if(RootValue) Result = AK_Json__Value_Copy(Context, RootValue);
    
    AK_Json__Arena_Delete(ParseArena);
    return Result;
}

/***********
*** Keys ***
************/

AK_JSON_DEF ak_json_str AK_Json_Key_Get_Name(ak_json_key* Key)
{
    return Key->Str;
}

AK_JSON_DEF ak_json_value* AK_Json_Key_Get_Value(ak_json_key* Key)
{
    return Key->Value;
}

/*************
*** Values ***
**************/
AK_JSON_DEF ak_json_value_type AK_Json_Value_Get_Type(ak_json_value* Value)
{
    return Value->Type;
}

AK_JSON_DEF ak_json_str AK_Json_Value_Get_String(ak_json_value* Value)
{
    AK_JSON_ASSERT(Value->Type == AK_JSON_VALUE_TYPE_STRING);
    return Value->String;
}

AK_JSON_DEF int AK_Json_Value_Get_Boolean(ak_json_value* Value)
{
    AK_JSON_ASSERT(Value->Type == AK_JSON_VALUE_TYPE_BOOLEAN);
    return Value->Boolean;
}

AK_JSON_DEF double AK_Json_Value_Get_Number(ak_json_value* Value)
{
    AK_JSON_ASSERT(Value->Type == AK_JSON_VALUE_TYPE_NUMBER);
    return Value->Number;
}

AK_JSON_DEF ak_json_array* AK_Json_Value_Get_Array(ak_json_value* Value)
{
    AK_JSON_ASSERT(Value->Type == AK_JSON_VALUE_TYPE_ARRAY);
    return &Value->Array;
}

AK_JSON_DEF ak_json_object* AK_Json_Value_Get_Object(ak_json_value* Value)
{
    AK_JSON_ASSERT(Value->Type == AK_JSON_VALUE_TYPE_OBJECT);
    return &Value->Object;
}

#endif
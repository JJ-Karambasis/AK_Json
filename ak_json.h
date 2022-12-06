#ifndef AK_JSON_H
#define AK_JSON_H

typedef unsigned char ak_u8;
typedef unsigned long long ak_u64;
typedef ak_u64 ak_json_user_data;

typedef struct ak_json_str
{
    const ak_u8* Str;
    const ak_u64 Length;
} ak_json_str;

#define AK_Json_Str(str) {(const ak_u8*)str, sizeof(str)-1}

typedef struct ak_json_allocator ak_json_allocator;
typedef struct ak_json_context   ak_json_context;
typedef struct ak_json_node      ak_json_node;

typedef void* ak_json_alloc(ak_json_allocator* Allocator, unsigned int Size);
typedef void  ak_json_free(ak_json_allocator* Allocator, void* Memory);

typedef struct ak_json_allocator
{
    ak_json_alloc*    Allocate;
    ak_json_free*     Free;
    ak_json_user_data UserData;
} ak_json_allocator;

ak_json_context* AK_Json_Create(ak_json_allocator* Allocator);
ak_json_node*    AK_Json_Parse(ak_json_context* Context, ak_json_str Str);
void             AK_Json_Delete(ak_json_context* Context);

#endif

#ifdef AK_JSON_IMPLEMENTATION

#ifndef AK_JSON_ASSERT
#include <assert.h>
#define AK_JSON_ASSERT(cond) assert(cond)
#endif

#if !defined(AK_JSON_MALLOC) || !defined(AK_JSON_FREE)
#include <stdlib.h>

#undef AK_JSON_MALLOC
#undef AK_JSON_FREE

#define AK_JSON_MALLOC(size) malloc(size)
#define AK_JSON_FREE(memory) free(memory)
#endif

typedef struct ak_json__arena_block
{
    ak_u8* Memory;
    ak_u64 Used;
    ak_u64 Size;
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

ak_json__arena* AK_Json__Arena_Create(ak_json_allocator Allocator, unsigned int InitialBlockSize)
{
    unsigned int AllocationSize = InitialBlockSize+sizeof(ak_json__arena)+sizeof(ak_json__arena_block);
    ak_json__arena* Arena = (ak_json__arena*)Allocator.Alloc(&Allocator, AllocationSize);
    if(!Arena) 
    {
        //TODO(JJ): Log out of memory
        return NULL;
    }
    
    Arena->Allocator = Allocator;
    Arena->InitialBlockSize = InitialBlockSize;
    Arena->FirstBlock = Arena->LastBlock = Arena->CurrentBlock = (ak_json__arena_block*)(Arena+1);
    Arena->CurrentBlock->Memory = (ak_u8*)(Arena->CurrentBlock+1);
    Arena->CurrentBlock->Used = 0;
    Arena->CurrentBlock->Size = InitialBlockSize;
    Arena->CurrentBlock->Next = NULL;
    
    return Arena;
}

void AK_Json__Arena_Delete(ak_json__arena* Arena)
{
    if(Arena && Arena->FirstBlock)
    {
        ak_json_allocator Allocator = Arena->Allocator;
        for(ak_json__arena_block* Block = Arena->FirstBlock->Next; Block;)
        {
            ak_json__arena* CurrentBlock = Block;
            Block = Block->Next;
            Allocator.Free(&Allocator, CurrentBlock);
        }
        Allocator.Free(&Allocator, Arena);
    }
}

ak_json__arena_block* AK_Json__Arena_Create_Block(ak_json__arena* Arena, unsigned int BlockSize)
{
    ak_json_allocator Arena->Allocator;
    ak_json__arena_block* Block = (ak_json__arena_block*)Allocator.Allocate(&Allocator, BlockSize+sizeof(ak_json__arena_block));
    Block->Memory = (ak_u8*)(Block+1);
    Block->Used = 0;
    Block->Size = BlockSize;
    Block->Next = NULL;
    return Block;
}

ak_json__arena_block* AK_Json__Arena_Get_Block(ak_json__arena* Arena, unsigned int Size)
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

void AK_Json__Arena_Add_Block(ak_json__arena* Arena, ak_json__arena_block* Block)
{
    ak_json__arena_block* Last = Arena->LastBlock;
    if(!Last)
    {
        Assert(!Arena->FirstBlock);
        Arena->FirstBlock = Arena->LastBlock = Block;
    }
    else
    {
        Last->Next = Block;
        Arena->LastBlock = Block;
    }
}

void* AK_Json__Arena_Push(ak_json__arena* Arena, unsigned int Size)
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

#endif
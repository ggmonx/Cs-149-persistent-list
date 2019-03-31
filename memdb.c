#include<stdio.h>
#include "memdb.h"
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
/*******************************************************
 * name: Gaston Garrido
 * assignment: persistent list
 * class: cs 149
********************************************************/
//Unix only, not portable
//Going to adopt a first fit policy and grow to max size when there is < 57 bytes of free space left
//if not enough space print out cannot add to list delete something first


/*
void list
parameters:
fhdr: head of linked lists, type is struct fhdr_s *

size: entry to be linked in free list, type is offt
output: None

list takes a size and uses it along with fhdr to generate a pointer to the end of file
list then iterates through the data linked list until the end of the data list occurs,
printing out the strings in order(strings are added in sorted order so ant comes before boat)
*/
void list(struct fhdr_s *fhdr, off_t size)
{
    bool print=false;
    void *end_of_file = (char *) fhdr + size;
    struct entry_s *entry;
    if(fhdr->data_start==0)//nothing in db
    {
        return;
    }
    else
    {
        entry=(struct entry_s *) ((char *) fhdr + fhdr->data_start);
    }
    while((void*)entry<end_of_file)
    {
        if(entry->magic==ENTRY_MAGIC_DATA)
        {
            print=true;
            printf("%s\n",entry->str);
            fflush(stdout);
        }
        if(entry->next>0)
        {
            entry = (struct entry_s *) ((char *) fhdr + entry->next);
        }
        else
        {
            if(entry->magic==ENTRY_MAGIC_DATA && entry->next==0)//next entry is zero
            {
                return;
            }
        }
    }
}
/*
bool contains
parameters:
fhdr: head of linked lists, type is struct fhdr_s *

size: entry to be linked in free list, type is offt

test: string to be checked, type is char[]

b: boolean that lets program know when to print,
used to ensure different things get printed for add and delete. type is bool

output: True if list contains test, False if list does not contain test

contains generates an end of file pointer with size and fhdr.
contains then sets entry to be the first entry in the list(could be free space)
contains then iterates through the linked list as a list until end of file and tests the entries
with the correct magic number to see if they are the string, if string is found
return true else at the end return false.
*/
bool contains(struct fhdr_s *fhdr, off_t size,char test[], bool b)
{
    void *end_of_file = (char *) fhdr + size;
    struct entry_s *entry = (struct entry_s *) ((char *) fhdr + sizeof(*fhdr));//taken from example memdb
    while((void*)entry<end_of_file)
    {
        if(entry->magic==ENTRY_MAGIC_DATA)
        {
             if(strcmp(entry->str,test)==0)
            {
                if(!b)
                {
                    printf("error: list already contains %s\n",test);
                }
                return true;
            } 
        }
        entry = (struct entry_s *) (entry->str + entry->len);
    }
    if(b)
    {
        printf("error: list does not contain %s\n",test);
    }
    return false;
}
/*
void linkFreeList
parameters:
fhdr: head of linked lists, type is struct fhdr_s *

nextEntry: entry to be linked in free list, type is struct entry_s *

output: None

linkFreeList creates entry to be the first entry in the free list
linkFreeList checks if nextEntry comes before entry according to pointer order
if it does the insert nextEntry at the head of the free list.
Otherwise linkFreeList iterates over free linked list until it finds a spot for nextEntry,
either between two entries or at the end of the free list
*/
void linkFreeList(struct fhdr_s *fhdr,struct entry_s * nextEntry)
{
    struct entry_s *entry=(struct entry_s*)((moffset_t)fhdr+fhdr->free_start);
    if(nextEntry<entry)//nextEntry belongs at the head
    {
        fhdr->free_start=(moffset_t)nextEntry-(moffset_t)fhdr;
        nextEntry->next=(moffset_t)entry-(moffset_t)fhdr;
        return;
    }
    while(entry->next>0)
    {
        struct entry_s *next=(struct entry_s*)((moffset_t)fhdr+(entry->next));
        if(next>nextEntry&&nextEntry<entry)//nextEntry goes between
        {
            nextEntry->next=entry->next;
            entry->next=(moffset_t)nextEntry-(moffset_t)fhdr;
            return;
        }
        entry=(struct entry_s*)((moffset_t)fhdr+entry->next);
    }
    //last entry in free list
    if(nextEntry>entry)
    {
        entry->next=(moffset_t)nextEntry-(moffset_t)fhdr;
    }
}
/*
void unlinkEntry
parameters:
nextEntry: entry to be unlinked in free list, type is struct entry_s *

fhdr: head of linked lists, type is struct fhdr_s *

output: None

unlinkEntry unlinks the entry to be changed to data from the free list.
if nextEntry is the first entry fhdr will be changed to point to what nextEntry is pointing to
else unlinkEntry iterates through the free list until the end or it finds what points to nextEntry
and unlinks it then it returns.
*/
void unlinkEntry(struct entry_s *nextEntry,struct fhdr_s *fhdr)
{
    if(fhdr->free_start==(moffset_t) nextEntry - (moffset_t) fhdr)//nextEntry is head of free linkedList
    {
        fhdr->free_start=nextEntry->next;
        return;
    }
    else
    {
        struct entry_s *entry=(struct entry_s *) ((char *) fhdr + fhdr->free_start);
        while(entry->next>0)
        {
            if(entry->next==(moffset_t) nextEntry - (moffset_t) fhdr)//found thing that points to nextEntry
            {
                entry->next=nextEntry->next;
                return;
            }
            entry=(struct entry_s *)((moffset_t)entry+(moffset_t)entry->next);
        }
    }  
}
/*
void linkEntry
parameters:

fhdr: head of linked lists, type is struct fhdr_s *

nextEntry: entry to be linked in data list, type is struct entry_s *

output: None

linkEntry makes entry equal to the first item in the data linked list
it checks if nextEntry can be added at the head of the data list and if so
makes fhdr point to nextEntry and nextEntry point to entry
else it iterates through the data list until it finds a spot for nextEntry between
two entries or it adds it as the tail.
*/
void linkEntry(struct fhdr_s *fhdr,struct entry_s *nextEntry)
{
    struct entry_s *entry = (struct entry_s *) ((char *) fhdr + fhdr->data_start);
    //case 1 can be added at head of linked list nextEntry<entry
        if(strcmp(nextEntry->str,entry->str)<0)
        {
            fhdr->data_start=((moffset_t)nextEntry-(moffset_t)fhdr);
            nextEntry->next=((moffset_t)entry-(moffset_t)fhdr);
            return;

        }

    else//not at head
    {
        struct entry_s *entry1;
        while(entry->next>0)
        {
            entry1=(struct entry_s *) ((char *) fhdr + entry->next);
            if(strcmp(nextEntry->str,entry->str)>0&&strcmp(nextEntry->str,entry1->str)<0)
            {// case where nextEntry goes between entry and entry1
                nextEntry->next=((moffset_t)entry1-(moffset_t)fhdr);
                entry->next=((moffset_t)nextEntry-(moffset_t)fhdr);
                return;
            }
            entry=(struct entry_s*)((moffset_t)entry->next+(moffset_t)fhdr);
        }//at last official entry in list
        if(entry->next==0)//kinda redundant check, since loop stops when entry's next is zero
        {
            if(strcmp(nextEntry->str,entry->str)>0)
            {
                entry->next=(moffset_t)nextEntry-(moffset_t)fhdr;
                return;
            }
        }
    }
    

}
/*
void linkEnd
parameters:

fhdr: head of linked lists, type is struct fhdr_s *

oldEntry: entry to be unlinked in data list, type is struct entry_s *

newEntry: entry to be linked in data list, type is struct entry_s *

output: None

linkEnd iterates through the free list until it finds what points to oldEntry and makes it point to
newEntry(this is done for adds that use the chunk of free space at end of the list)
*/
void linkEnd(struct fhdr_s *fhdr,struct entry_s *oldEntry,struct entry_s *newEntry)
{
    if(fhdr->free_start==((moffset_t)oldEntry-(moffset_t)fhdr))//oldEntry is head of free list
    {
        fhdr->free_start=(moffset_t)newEntry-(moffset_t)fhdr;
        return;
    }
    else
    {
        struct entry_s *entry=(struct entry_s *)((char*)fhdr+fhdr->free_start);
        while(entry->next>0)//found what points to oldEntry
        {
            if(entry->next==(moffset_t)oldEntry-(moffset_t)fhdr)
            {
                entry->next=(moffset_t)newEntry-(moffset_t)fhdr;
                return;
            }
        }
    }
}
/*
void coalesce
parameters:

fhdr: head of linked lists, type is struct fhdr_s *

size: size of file type is off_t

output: None

coalesce iterates through free list while not at end of file and adjacent free space to
the right is before end of file, if coalesce finds adjacent free space it coalesces them into 
one chunk and calls itself again until it goes one round without finding something to coalesce
*/
void coalesce(struct fhdr_s *fhdr,off_t size)
{
    struct entry_s *entry=(struct entry_s *)((char*) fhdr+fhdr->free_start);
    void *end_of_file = (char *) fhdr + size;
    while(entry->next>0&&(void*)((char*)entry+sizeof(struct entry_s)+entry->len)<end_of_file)
    {
        struct entry_s *entrysNext=(struct entry_s *)((char*)fhdr+entry->next);
        struct entry_s *temp=(struct entry_s *)((char*)entry+sizeof(struct entry_s)+entry->len);
        if(entrysNext==temp)//pointers are right next to each other
        {
            entry->next=entrysNext->next;
            entry->len=entry->len+sizeof(struct entry_s)+entrysNext->len;
            coalesce(fhdr,size);
            return;
        }
        entry=(struct entry_s *)((char*)fhdr+entry->next);
    }
}
/*
void add
parameters:

fhdr: head of linked lists, type is struct fhdr_s *

words: string to be added, type is char[]

size: size of file type is off_t

output: None
before add is called program checks if string exists via contains and if it does then add is not called
add iterates through the free list until it finds a chunk of memory that can be used or end of file is
reached. if end of file print error msg, otherwise call unlink entry if free space is not end of free list
it then checks if the memory can be split if so it links the two new pieces of memory from the split after a
assigning the correct data to the entry(entries)
after that it calls coalesce. If there is no preexisting data in db it makes fhdr pt to the new entry
*/
void add(struct fhdr_s *fhdr,char words[],off_t size)
{
    bool lastFree=false;
    void *end_of_file = (char *) fhdr + size;
    bool changeRemaining=false;
    struct entry_s *nextEntry =(struct entry_s*) ((char*) fhdr+ fhdr->free_start);
    while(nextEntry->len<strlen(words)+1)
    {
        if(nextEntry->next==0)//reached end can't add
        {
            return;
        }
        else
        {
            nextEntry=(struct entry_s*) ((char*) fhdr+ nextEntry->next);
        } 
    }
    if(nextEntry->next>0)
    {
        unlinkEntry(nextEntry,fhdr);
    }
    else
    {
        lastFree=true;
    }
    if(nextEntry->len>=strlen(words)+1)//can add 
    {
        nextEntry->magic=ENTRY_MAGIC_DATA;
        int remaining=nextEntry->len-strlen(words)-sizeof(*nextEntry)-1;
        if(remaining>19)
        {
            changeRemaining=true;
        }
        strcpy(nextEntry->str,words);
        if(changeRemaining)
        {
            nextEntry->len=strlen(words)+1;
            struct entry_s* entry2=(struct entry_s*)((char*)nextEntry+sizeof(*nextEntry)+nextEntry->len);
            entry2->magic=ENTRY_MAGIC_FREE;
            entry2->next=0;
            entry2->len=remaining;
            if(lastFree)
            {
                linkEnd(fhdr,nextEntry,entry2);
            }
            else
            {
                linkFreeList(fhdr,entry2);
                
            }
                coalesce(fhdr,size);
        }
        nextEntry->next=0;
        if(fhdr->data_start==0)//nothing in db
        {
            fhdr->data_start=((moffset_t)nextEntry-(moffset_t)fhdr);
            return;
        }
        linkEntry(fhdr,nextEntry);
    }
    else
    {
        printf("Not enough space to add, delete something first\n");
    }
}
/*
void unlinkDelete
parameters:

fhdr: head of linked lists, type is struct fhdr_s *

entry: entry to be unlinked, type is struct entry_s *

output: None

unlinkDelete searches through the data list until it finds the correct entry and makes it so that whatever
pointed to entry now points to what entry points to(a->b->c, call unlink on b it is now a->c)
*/
void unlinkDelete(struct fhdr_s *fhdr, struct entry_s *entry)//goal is to unlink entry from data list
{
    if(fhdr->data_start==(moffset_t) entry - (moffset_t) fhdr)//entry is head of data linkedList
    {
        fhdr->data_start=entry->next;
        return;
    }
    else
    {
        struct entry_s *entry1=(struct entry_s *) ((char *) fhdr + fhdr->data_start);
        while(entry1->next>0)
        {
            if(entry1->next==(moffset_t) entry - (moffset_t)fhdr)//found thing that points to entry
            {
                entry1->next=entry->next;
                return;
            }
            entry1=(struct entry_s *) ((char *) fhdr + entry1->next);
        }
    }
}
/*
void linkDelete
parameters:
fhdr: head of linked lists, type is struct fhdr_s *

nextEntry: entry to be linked in free list, type is struct entry_s *
output: None
linkDelete iterates through free list until it finds an entry that is greater than nextEntry
according to the pointers it then links properly such that something points to entry entry points to
something and the free linked list is sorted by pointer order
*/
void linkDelete(struct fhdr_s *fhdr,struct entry_s *nextEntry)
{
    struct entry_s *entry=(struct entry_s *)((char*)fhdr+fhdr->free_start);
    if((moffset_t)nextEntry<(moffset_t)entry)// next entry comes before entry, add to head of free list
    {
        fhdr->free_start=(moffset_t)nextEntry-(moffset_t)fhdr;
        nextEntry->next=(moffset_t)entry-(moffset_t)fhdr;
        return;
    }
    else
    {
        while(entry->next>0)
        {
           struct entry_s *nexts=(struct entry_s *)(entry->next+(char*)fhdr);
            
            if((moffset_t)nextEntry>(moffset_t)entry && (moffset_t)nextEntry<(moffset_t)nexts)//nextEntry is between entry and its next
            {
                nextEntry->next=entry->next;
                entry->next=(moffset_t)nextEntry-(moffset_t)fhdr;
                return;
            }
            entry=(struct entry_s *)((char*)fhdr+entry->next);
        }
        //at last entry in free list should always have been returned before here
        if((moffset_t)nextEntry-(moffset_t)entry<0)//nextEntry is last in list
        {
            entry->next=(moffset_t)nextEntry-(moffset_t)fhdr;
        }
    }
}
/*
void delete
contains is called before delete, delete is called only if list contains words
parameters:
fhdr: head of linked lists, type is struct fhdr_s *

words: string to be deleted, type is char[]

size: size of file,type is off_t

output: None
delete iterates through the data list until it finds the correct entry it then unlinks that entry from
data list and modifies it to be free space and then links it to the free list.
after that coalesce is called.
*/
void delete(struct fhdr_s *fhdr,char words[],off_t size)//redo delete tomorrow
{
    void *end_of_file = (char *) fhdr + size;
    struct entry_s *entry = (struct entry_s *) ((char *) fhdr + fhdr->data_start);//taken from example memdb
    while((void*)entry<end_of_file && entry->magic==ENTRY_MAGIC_DATA&&strcmp(entry->str,words)!=0)
    {
    entry = (struct entry_s *) (entry->next+(moffset_t)fhdr);
    } //found right location
    unlinkDelete(fhdr,entry);
    entry->magic=ENTRY_MAGIC_FREE;
    entry->next=0;
    strcpy(entry->str,"");
    linkDelete(fhdr,entry);
    coalesce(fhdr,size);
}
/*
void changeSize
parameters:
fd: file descriptor, type is int

fhdr: head of linked lists, type is struct fhdr_s *

output: None
changeSize calculates total amount of freeSpace and if less than a certain amount(57) is left
and file size is less than max, file is grown to max and the last entry in the free list
is updated accordingly
*/
void changeSize(int fd,struct fhdr_s *fhdr)
{
    struct stat s1;
    fstat(fd,&s1);
    int freeSpace=0;
    struct entry_s *entry=(struct entry_s *)((moffset_t)fhdr+fhdr->free_start);
    while(entry->next>0)
    {
        freeSpace=freeSpace+entry->len;
        entry=(struct entry_s *)((char*)fhdr+entry->next);
    }
    freeSpace=freeSpace+entry->len;
    if(freeSpace<(19*3)&&s1.st_size<MAX_SIZE)//less than 3 avg allocations left
    {
        ftruncate(fd,MAX_SIZE);
        fstat(fd,&s1);
        entry->len=entry->len+(MAX_SIZE-INIT_SIZE);
    }
}

static bool temporary=false;

int main(int argc, char *argv[])
{ 
    int i=1;
    int strlength=0;
    if(argc<2)
    {
        printf("USAGE: %s dbfile\n", argv[0]);
        return 1;
    }
        if(argc==3)
        {
            if(strcmp(argv[1],"-t")==0)
            {
                i++;
                temporary=true;
            }
        }
        size_t s=0;
        char *line;
        char *token;
        struct fhdr_s *beginning;
        struct entry_s *current;
        int fd=open(argv[i],O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
        struct stat s1;
        fstat(fd,&s1);

    if(s1.st_size<INIT_SIZE)
    {
        ftruncate(fd,INIT_SIZE);
    }
    fstat(fd,&s1);
    if(!temporary)
    { 
        beginning=mmap(NULL,MAX_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
        //printf("mapping is shared\n");
    } 
    else
    {
        beginning=mmap(NULL,MAX_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
        //printf("mapping is private\n");
    } 
     if (beginning == (void *) -1) {// error check gotten from dbdump from prof
        perror(argv[i]);
        return 4;
    }
   if(beginning->magic!=FILE_MAGIC)// sets first entry to be free space if file is new
   {
    beginning->magic=FILE_MAGIC;
    beginning->data_start=0;
    beginning->free_start=20; 
    struct entry_s *entry = (struct entry_s *) ((char *) beginning + sizeof(*beginning));
    entry->next=0;
    entry->magic=0;
    entry->len=s1.st_size-sizeof(*beginning)-sizeof(*entry);
   }

    while(s=getline(&line,&s,stdin)!=-1)
    {

        strlength=strlen(line);
        bool check=((strcmp(line,"a\n")==0 || strcmp(line,"d\n")==0) && strcmp(line,"l\n")!=0);
        token=strtok(line," ");
        if(check || (strcmp(token,"a")!=0 && strcmp(token,"d")!=0 && strcmp(line,"l\n")!=0))
        {// checks if command is valid
            printf("command must be one of:\n a string_to_add\n d string_to_del\n l\n");
            fflush(stdout);
        }
        else if(strcmp(line,"l\n")==0)
        {
            list(beginning,s1.st_size);
        }
        
        if(strcmp(token,"a")==0)
        {
            token=strtok(NULL,"\n");
            strcpy(line,"");
            while(token!=NULL)
            {
                strcat(line,token);
                token=strtok(NULL,"\n");
            }
            if(!contains(beginning,s1.st_size,line,false))
            {
                add(beginning,line,s1.st_size);
                if(s1.st_size<MAX_SIZE)
                {
                    changeSize(fd,beginning);
                }
                fstat(fd,&s1);
            }
        
        }
        else if(strcmp(token,"d")==0)
        {
            token=strtok(NULL,"\n");
            strcpy(line,"");
            while(token!=NULL)
            {
                strcat(line,token);
                token=strtok(NULL,"\n");
            }
            
            if(contains(beginning,s1.st_size,line,true))
            {
                delete(beginning,line,s1.st_size);
            }
        }
    }
}
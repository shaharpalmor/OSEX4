#include <stdio.h>
#include <stdlib.h>
#include "threadPool.h"


void hello (void* a)
{
   printf("hello\n");
}

void func1(){
    int i,j;
    for(i = 0;i<100;i++){
        j++;
    }
}


void test_thread_pool_sanity()
{
   int i;
   
   ThreadPool* tp = tpCreate(5);
   
   for(i=0; i<5; ++i)
   {
      tpInsertTask(tp,hello,NULL);
   }
    //tpInsertTask(tp,func1,NULL);
    //tpInsertTask(tp,func1,NULL);
    //tpInsertTask(tp,func1,NULL);

    //sleep(5);

   tpDestroy(tp,0);
}

/*
int main()
{
   test_thread_pool_sanity();

   return 0;
}
*/
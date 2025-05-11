#include "../Inc/cat.h"




static void sayHello(void *cat)
{
    Cat *cat = (Cat *)cat;
    printf("hello, %s\n", cat->name);
}

void initCat(Cat *cat, char *name, int age)
{
    strcpy(cat->name, name);
    cat->age = age;
    cat->sayHello = sayHello;
}
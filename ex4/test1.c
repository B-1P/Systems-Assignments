#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

int test1(void) {
    List *head = create_node(3);
    head = add_node(head, 4);
    char *result = tostring(head);
    deallocateAll(head);
    if(strcmp(result, "43") != 0) {
        printf("Test1 failed: expected 43 got %s\n", result);
        return 0;
    } else {
	free(result);
        printf("Test1 succeeded\n");
        return 1;
    }
}

int test2(void){
    List *head = create_node(3);
    head = add_node(head, 4);
    head = add_node(head, 5);
    head = add_node(head, 6);
    head = remove_node(head, 5);
    char *result = tostring(head);
    deallocateAll(head);
    if(strcmp(result, "643") != 0) {
        printf("Test1 failed: expected 643 got %s\n", result);
        return 0;
    } else {
	free(result);
        printf("Test2 succeeded\n");
        return 1;
    }

}

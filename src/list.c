#include <stdlib.h>
#include "list.h"

void __list_add(struct list_head *curr, struct list_head *prev, struct list_head *next) {
	prev->next = curr;
	next->prev = curr;
	curr->prev = prev;
	curr->next = next;
}

void list_add(struct list_head *curr, struct list_head *head) {
	__list_add(curr, head->prev, head);
}

void list_add_tail(struct list_head *curr, struct list_head *head) {
	__list_add(curr, head, head->next);
}

void __list_del(struct list_head *prev, struct list_head *next) {
	prev->next = next;
	next->prev = prev;
}

void list_del(struct list_head *curr) {
	__list_del(curr->prev, curr->next);
	curr->prev = curr->next = NULL;
}

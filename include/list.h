#ifndef __LIST_H__
#define __LIST_H__
#define INIT_LIST_HEAD(name) {&(name), &(name)}

#define LIST_FOR_EACH(pos, head) \
		for (pos = (head)->next; pos != head; pos = pos->next)

#define CONTAINER_OF(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member) );})

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

void __list_add(struct list_head *, struct list_head *, struct list_head *);

void list_add(struct list_head *, struct list_head *);

void list_add_tail(struct list_head *, struct list_head *);

void __list_del(struct list_head *, struct list_head *);

void list_del(struct list_head *);

#endif

/*
 * Collections-C
 * Copyright (C) 2013-2015 Srđan Panić <srdja.panic@gmail.com>
 *
 * This file is part of Collections-C.
 *
 * Collections-C is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Collections-C is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Collections-C.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CC_SLIST_H
#define CC_SLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cc_common.h"

/**
 * A singly linked list. List is a sequential structure that
 * supports constant time insertion, deletion  and lookup at
 * the beginning of the list, while the worst case for these
 * operations is linear time.
 */
typedef struct cc_slist_s CC_SList;

/**
 * CC_SList node.
 *
 * @note Modifying the links may invalidate the list structure.
 */
typedef struct snode_s {
    void *data;
    struct snode_s *next;
} SNode;

/**
 * CC_SList iterator structure. Used to iterate over the elements
 * of the list in an ascending order. The iterator also supports
 * operations for safely adding and removing elements during iteration.
 */
typedef struct cc_slist_iter_s {
    size_t index;
    CC_SList *list;
    SNode *next;
    SNode *current;
    SNode *prev;
} CC_SListIter;

/**
 * CC_SList zip iterator structure. Used to iterate over two CC_SLists in
 * lockstep in an ascending order until one of the lists is exhausted.
 * The iterator also supports operations for safely adding and
 * removing elements during iteration.
 */
typedef struct cc_slist_zip_iter_s {
    size_t index;
    CC_SList *l1;
    CC_SList *l2;
    SNode *l1_next;
    SNode *l2_next;
    SNode *l1_current;
    SNode *l2_current;
    SNode *l1_prev;
    SNode *l2_prev;
} CC_SListZipIter;

/**
 * CC_SList configuration structure. Used to initialize a new CC_SList with
 * specific values.
 */
typedef struct cc_slist_conf_s {
    void *(*mem_alloc)(size_t size);
    void *(*mem_calloc)(size_t blocks, size_t size);
    void (*mem_free)(void *block);
} CC_SListConf;

void cc_slist_conf_init(CC_SListConf *conf);
enum cc_stat cc_slist_new(CC_SList **list);
enum cc_stat cc_slist_new_conf(CC_SListConf const *const conf, CC_SList **list);
void cc_slist_destroy(CC_SList *list);
void cc_slist_destroy_cb(CC_SList *list, void (*cb)(void *));
size_t cc_slist_struct_size();

enum cc_stat cc_slist_splice(CC_SList *list1, CC_SList *list2);
enum cc_stat cc_slist_splice_at(CC_SList *list1, CC_SList *list2, size_t index);

enum cc_stat cc_slist_add(CC_SList *list, void *element);
enum cc_stat cc_slist_add_at(CC_SList *list, void *element, size_t index);
enum cc_stat cc_slist_add_all(CC_SList *list1, CC_SList *list2);
enum cc_stat cc_slist_add_all_at(CC_SList *list1, CC_SList *list2, size_t index);
enum cc_stat cc_slist_add_first(CC_SList *list, void *element);
enum cc_stat cc_slist_add_last(CC_SList *list, void *element);

enum cc_stat cc_slist_remove(CC_SList *list, void *element, void **out);
enum cc_stat cc_slist_remove_first(CC_SList *list, void **out);
enum cc_stat cc_slist_remove_last(CC_SList *list, void **out);
enum cc_stat cc_slist_remove_at(CC_SList *list, size_t index, void **out);

enum cc_stat cc_slist_remove_all(CC_SList *list);
enum cc_stat cc_slist_remove_all_cb(CC_SList *list, void (*cb)(void *));

enum cc_stat cc_slist_get_at(CC_SList *list, size_t index, void **out);
enum cc_stat cc_slist_get_first(CC_SList *list, void **out);
enum cc_stat cc_slist_get_last(CC_SList *list, void **out);

enum cc_stat cc_slist_sublist(CC_SList *list, size_t from, size_t to, CC_SList **out);
enum cc_stat cc_slist_copy_shallow(CC_SList *list, CC_SList **out);
enum cc_stat cc_slist_copy_deep(CC_SList *list, void *(*cp)(void *), CC_SList **out);

enum cc_stat cc_slist_replace_at(CC_SList *list, void *element, size_t index, void **out);

size_t cc_slist_contains(CC_SList *list, void *element);
size_t cc_slist_contains_value(CC_SList *list, void *element, int (*cmp)(const void *, const void *));
enum cc_stat cc_slist_index_of(CC_SList *list, void *element, size_t *index);
enum cc_stat cc_slist_to_array(CC_SList *list, void ***out);

void cc_slist_reverse(CC_SList *list);
enum cc_stat cc_slist_sort(CC_SList *list, int (*cmp)(void const *, void const *));
size_t cc_slist_size(CC_SList *list);

void cc_slist_foreach(CC_SList *list, void (*op)(void *));

enum cc_stat cc_slist_filter(CC_SList *list, bool (*predicate)(const void *), CC_SList **out);
enum cc_stat cc_slist_filter_mut(CC_SList *list, bool (*predicate)(const void *));

void cc_slist_iter_init(CC_SListIter *iter, CC_SList *list);
enum cc_stat cc_slist_iter_remove(CC_SListIter *iter, void **out);
enum cc_stat cc_slist_iter_add(CC_SListIter *iter, void *element);
enum cc_stat cc_slist_iter_replace(CC_SListIter *iter, void *element, void **out);
enum cc_stat cc_slist_iter_next(CC_SListIter *iter, void **out);
size_t cc_slist_iter_index(CC_SListIter *iter);

void cc_slist_zip_iter_init(CC_SListZipIter *iter, CC_SList *l1, CC_SList *l2);
enum cc_stat cc_slist_zip_iter_next(CC_SListZipIter *iter, void **out1, void **out2);
enum cc_stat cc_slist_zip_iter_add(CC_SListZipIter *iter, void *e1, void *e2);
enum cc_stat cc_slist_zip_iter_remove(CC_SListZipIter *iter, void **out1, void **out2);
enum cc_stat cc_slist_zip_iter_replace(CC_SListZipIter *iter, void *e1, void *e2, void **out1, void **out2);
size_t cc_slist_zip_iter_index(CC_SListZipIter *iter);

#define CC_SLIST_FOREACH(val, slist, body)                                               \
    {                                                                                    \
        CC_SListIter cc_slist_iter_53d46d2a04458e7b;                                     \
        cc_slist_iter_init(&cc_slist_iter_53d46d2a04458e7b, slist);                      \
        void *val;                                                                       \
        while (cc_slist_iter_next(&cc_slist_iter_53d46d2a04458e7b, &val) != CC_ITER_END) \
            body                                                                         \
    }

#define CC_SLIST_FOREACH_ZIP(val1, val2, slist1, slist2, body)                                            \
    {                                                                                                     \
        CC_SListZipIter cc_slist_zip_iter_ea08d3e52f25883b3;                                              \
        cc_slist_zip_iter_init(&cc_slist_zip_iter_ea08d3e52f25883b, slist1, slist2);                      \
        void *val1;                                                                                       \
        void *val2;                                                                                       \
        while (cc_slist_zip_iter_next(&cc_slist_zip_iter_ea08d3e52f25883b3, &val1, &val2) != CC_ITER_END) \
            body                                                                                          \
    }

struct cc_slist_s {
    size_t size;
    SNode *head;
    SNode *tail;

    void *(*mem_alloc)(size_t size);
    void *(*mem_calloc)(size_t blocks, size_t size);
    void (*mem_free)(void *block);
};

static void *unlinkn(CC_SList *list, SNode *node, SNode *prev);
static bool unlinkn_all(CC_SList *list, void (*cb)(void *));
static void splice_between(CC_SList *list1, CC_SList *list2, SNode *base, SNode *end);
static bool link_all_externally(CC_SList *list, SNode **h, SNode **t);
static enum cc_stat get_node_at(CC_SList *list, size_t index, SNode **node, SNode **prev);
static enum cc_stat get_node(CC_SList *list, void *element, SNode **node, SNode **prev);

/**
 * Initializes the fields CC_SListConf struct to default values.
 *
 * @param[in] conf the CC_SListConf struct that is being initialized.
 */
void cc_slist_conf_init(CC_SListConf *conf) {
    conf->mem_alloc = malloc;
    conf->mem_calloc = calloc;
    conf->mem_free = free;
}

/**
 * Creates a new empty list and returns a status code.
 *
 * @param[out] out Pointer to a CC_SList that is being created.
 *
 * @return CC_OK if the creation was successful, or CC_ERR_ALLOC if the
 * memory allocation for the new CC_SList structure failed.
 */
enum cc_stat cc_slist_new(CC_SList **out) {
    CC_SListConf conf;
    cc_slist_conf_init(&conf);
    return cc_slist_new_conf(&conf, out);
}

/**
 * Creates a new empty CC_SList based on the specified CC_SListConf struct and
 * returns a status code.
 *
 * The CC_SList is allocated using the allocators specified in the CC_SListConf
 * struct. The allocation may fail if the underlying allocator fails.
 *
 * @param[in] conf CC_SList configuration struct. All fields must be initialized
 *            to appropriate values.
 *
 * @param[out] out Pointer to a CC_SList that is being createdo
 *
 * @return CC_OK if the creation was successful, or CC_ERR_ALLOC if the
 * memory allocation for the new CC_SList structure failed.
 */
enum cc_stat cc_slist_new_conf(CC_SListConf const *const conf, CC_SList **out) {
    CC_SList *list = (CC_SList *)conf->mem_calloc(1, sizeof(CC_SList));

    if (!list)
        return CC_ERR_ALLOC;

    list->mem_alloc = conf->mem_alloc;
    list->mem_calloc = conf->mem_calloc;
    list->mem_free = conf->mem_free;

    *out = list;
    return CC_OK;
}

/**
 * Destroys the list structure, but leaves the data that is holds intact.
 *
 * @param[in] list CC_SList that is to be destroyed
 */
void cc_slist_destroy(CC_SList *list) {
    cc_slist_remove_all(list);
    list->mem_free(list);
}

/**
 * Destroys the list structure along with all the data it holds.
 *
 * @note
 * This function should not be called on a list that has some of it's elements
 * allocated on the stack.
 *
 * @param[in] list CC_SList that is to be destroyed
 */
void cc_slist_destroy_cb(CC_SList *list, void (*cb)(void *)) {
    cc_slist_remove_all_cb(list, cb);
    list->mem_free(list);
}

/**
 * Adds a new element to the list. The element is appended to the list making it
 * the last element of the list.
 *
 * @param[in] list CC_SList to which the element is being added
 * @param[in] element element that is being added
 *
 * @return CC_OK if the element was successfully added, or CC_ERR_ALLOC if the
 * memory allocation for the new element has failed.
 */
enum cc_stat cc_slist_add(CC_SList *list, void *element) {
    return cc_slist_add_last(list, element);
}

/**
 * Prepends a new element to the list (adds a new "head") making it the first
 * element of the list.
 *
 * @param[in] list CC_SList to which the element is being added
 * @param[in] element element that is being added
 *
 * @return CC_OK if the element was successfully added, or CC_ERR_ALLOC if the
 * memory allocation for the new element has failed.
 */
enum cc_stat cc_slist_add_first(CC_SList *list, void *element) {
    SNode *node = (SNode *)list->mem_calloc(1, sizeof(SNode));

    if (!node)
        return CC_ERR_ALLOC;

    node->data = element;

    if (list->size == 0) {
        list->head = node;
        list->tail = node;
    } else {
        node->next = list->head;
        list->head = node;
    }
    list->size++;
    return CC_OK;
}

/**
 * Appends a new element to the list (adds a new "tail") making it the last
 * element of the list.
 *
 * @param[in] list CC_SList to which the element is being added
 * @param[in] element element that is being added
 *
 * @return CC_OK if the element was successfully added, or CC_ERR_ALLOC if the
 * memory allocation for the new element has failed.
 */
enum cc_stat cc_slist_add_last(CC_SList *list, void *element) {
    SNode *node = (SNode *)list->mem_calloc(1, sizeof(SNode));

    if (!node)
        return CC_ERR_ALLOC;

    node->data = element;

    if (list->size == 0) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->size++;
    return CC_OK;
}

/**
 * Adds a new element at the specified location in the CC_SList and shifts all
 * subsequent elements by one. The index at which the new element is being
 * added must be within the bounds of the list.
 *
 * @note This operation cannot be performed on an empty list.
 *
 * @param[in] list CC_SList to which this element is being added
 * @param[in] element element that is being added
 * @param[in] index the position in the list at which the new element is being
 *                  added
 *
 * @return CC_OK if the element was successfully added, CC_ERR_OUT_OF_RANGE if
 * the specified index was not in range, or CC_ERR_ALLOC if the memory
 * allocation for the new element failed.
 */
enum cc_stat cc_slist_add_at(CC_SList *list, void *element, size_t index) {
    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list, index, &node, &prev);

    if (status != CC_OK)
        return status;

    SNode *new_node = (SNode *)list->mem_calloc(1, sizeof(SNode));

    if (!new_node)
        return CC_ERR_ALLOC;

    new_node->data = element;

    if (!prev) {
        new_node->next = list->head;
        list->head = new_node;
    } else {
        SNode *tmp = prev->next;
        prev->next = new_node;
        new_node->next = tmp;
    }

    list->size++;
    return CC_OK;
}

/**
 * Adds all elements from the second list to the first. The elements from the
 * second list are added after the last element of the first list.
 *
 * @param[in] list1 CC_SList to which the elements are being added
 * @param[in] list2 CC_SList from which the elements are being taken
 *
 * @return CC_OK if the elements where successfully added, or CC_ERR_ALLOC if
 * the memory allocation for the new elements failed.
 */
enum cc_stat cc_slist_add_all(CC_SList *list1, CC_SList *list2) {
    if (list2->size == 0)
        return CC_OK;

    SNode *head = NULL;
    SNode *tail = NULL;

    if (!link_all_externally(list2, &head, &tail))
        return CC_ERR_ALLOC;

    if (list1->size == 0) {
        list1->head = head;
        list1->tail = tail;
    } else {
        list1->tail->next = head;
        list1->tail = tail;
    }
    list1->size += list2->size;

    return CC_OK;
}

/**
 * Adds all element from the second list to the first at the specified position
 * by shifting all subsequent elements by the size of the second list. The index
 * must be within the range of the first list.
 *
 * @param[in] list1 CC_SList to which the elements are being added
 * @param[in] list2 CC_SList from which the elements are being taken
 * @param[in] index position in the first list at which the elements should be
 *                  added
 *
 * @return CC_OK if the elements were successfully added,
 * CC_ERR_INDEX_OUT_OF_BOUNDS if the index was out of range, or
 * CC_ERR_ALLOC if the memory allocation for the new elements failed.
 */
enum cc_stat cc_slist_add_all_at(CC_SList *list1, CC_SList *list2, size_t index) {
    if (list2->size == 0)
        return CC_OK;

    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list1, index, &node, &prev);

    if (status != CC_OK)
        return status;

    SNode *head = NULL;
    SNode *tail = NULL;

    if (!link_all_externally(list2, &head, &tail))
        return CC_ERR_ALLOC;

    if (!prev) {
        tail->next = node;
        list1->head = head;
    } else {
        prev->next = head;
        tail->next = node;
    }

    list1->size += list2->size;

    return CC_OK;
}

/**
 * Duplicates the structure of the list without directly attaching it to a
 * specific list. If the operation fails, everything is cleaned up and false
 * is returned to indicate the failure.
 *
 * @param[in] list the list whose structure is being duplicated
 * @param[in, out] h the pointer to which the new head will be attached
 * @param[in, out] t the pointer to which the new tail will be attached
 *
 * @return true if the operation was successful
 */
static bool link_all_externally(CC_SList *list, SNode **h, SNode **t) {
    SNode *ins = list->head;

    size_t i;
    for (i = 0; i < list->size; i++) {
        SNode *new_node = (SNode *)list->mem_calloc(1, sizeof(SNode));

        if (!new_node) {
            while (*h) {
                SNode *tmp = (*h)->next;
                list->mem_free(*h);
                *h = tmp;
            }
            return false;
        }

        new_node->data = ins->data;

        if (!*h) {
            *h = new_node;
            *t = new_node;
        } else {
            (*t)->next = new_node;
            *t = new_node;
        }

        ins = ins->next;
    }
    return true;
}

/**
 * Splices the two CC_SLists together by appending the second list to the
 * first. This function moves all the elements from the second list into
 * the first list, leaving the second list empty.
 *
 * @param[in] list1 The consumer list to which the elements are moved.
 * @param[in] list2 The producer list from which the elements are moved.
 *
 * @return CC_OK if the elements were successfully moved
 */
enum cc_stat cc_slist_splice(CC_SList *list1, CC_SList *list2) {
    if (list2->size == 0)
        return CC_OK;

    if (list1->size == 0) {
        list1->head = list2->head;
        list1->tail = list2->tail;
    } else {
        list1->tail->next = list2->head;
        list1->tail = list2->tail;
    }
    list1->size += list2->size;

    list2->head = NULL;
    list2->tail = NULL;
    list2->size = 0;

    return CC_OK;
}

/**
 * Splices the two CC_SLists together at the specified index of the first list.
 * this function moves all the elements from the second list into the first
 * list at the position specified by the <code>index</code> parameter. After
 * this operation the second list will be left empty.
 *
 * @param[in] list1 the consumer list to which the elements are moved
 * @param[in] list2 the producer list from which the elements are moved
 * @param[in] index the index in the first list after which the elements
 *                   from the second list should be inserted
 *
 * @return CC_OK if the elements were successfully moved, CC_ERR_OUT_OF_RANGE if
 * the index was not in range,
 */
enum cc_stat cc_slist_splice_at(CC_SList *list1, CC_SList *list2, size_t index) {
    if (list2->size == 0)
        return CC_OK;

    if (index >= list1->size)
        return CC_ERR_OUT_OF_RANGE;

    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list1, index, &node, &prev);

    if (status != CC_OK)
        return status;

    splice_between(list1, list2, prev, node);

    return CC_OK;
}

/**
 * Inserts the second list between the two nodes of the first list. If the left
 * node is NULL the head of the second list will be become the head of the first
 * list. Similarly if the right node is null the tail of the first list will
 * become the tail of the second list.
 *
 * @param[in, out] l1 the consumer list
 * @param[in, out] l2 the producer list
 * @param[in] left the node after which the elements are being added
 * @param[in] right the node behind which the elements are being added
 */
static INLINE void splice_between(CC_SList *l1, CC_SList *l2, SNode *base, SNode *end) {
    if (!base) {
        l2->tail->next = l1->head;
        l1->head = l2->head;
    } else if (!end) {
        l1->tail->next = l2->head;
        l1->tail = l2->tail;
    } else {
        base->next = l2->head;
        l2->tail->next = end;
    }
    l1->size += l2->size;

    l2->head = NULL;
    l2->tail = NULL;
    l2->size = 0;
}

/**
 * Removes the first occurrence of the element from the specified CC_SList
 * and optionally sets the out parameter to the value of the removed element.
 *
 * @param[in] list CC_SList from which the element is being removed
 * @param[in] element element that is being removed
 * @param[out] out Pointer to where the removed value is stored, or NULL
 *                 if it is to be ignored
 *
 * @return CC_OK if the element was successfully removed, or
 * CC_ERR_VALUE_NOT_FOUND if the element was not found.
 */
enum cc_stat cc_slist_remove(CC_SList *list, void *element, void **out) {
    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node(list, element, &node, &prev);

    if (status != CC_OK)
        return status;

    void *val = unlinkn(list, node, prev);

    if (out)
        *out = val;

    return CC_OK;
}

/**
 * Removes the element at the specified index and optionally sets
 * the out parameter to the value of the removed element. The index
 * must be within the bounds of the list.
 *
 * @param[in] list  CC_SList from which the element is being removed
 * @param[in] index Index of the element that is being removed. Must be be
 *                  within the index range of the list.
 * @param[out] out  Pointer to where the removed value is stored,
 *                  or NULL if it is to be ignored
 *
 * @return CC_OK if the element was successfully removed, or CC_ERR_OUT_OF_RANGE
 * if the index was out of range.
 */
enum cc_stat cc_slist_remove_at(CC_SList *list, size_t index, void **out) {
    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list, index, &node, &prev);

    if (status != CC_OK)
        return status;

    void *e = unlinkn(list, node, prev);

    if (out)
        *out = e;

    return CC_OK;
}

/**
 * Removes the first (head) element of the list and optionally sets the out
 * parameter to the value of the removed element.
 *
 * @param[in] list CC_SList from which the first element is being removed
 * @param[out] out Pointer to where the removed value is stored, or NULL if it is
 *                 to be ignored
 *
 * @return CC_OK if the element was successfully removed, or CC_ERR_VALUE_NOT_FOUND
 * if the list is empty.
 */
enum cc_stat cc_slist_remove_first(CC_SList *list, void **out) {
    if (list->size == 0)
        return CC_ERR_VALUE_NOT_FOUND;

    void *e = unlinkn(list, list->head, NULL);

    if (out)
        *out = e;

    return CC_OK;
}

/**
 * Removes the last (tail) element of the list and optionally sets the out
 * parameter to the value of the removed element.
 *
 * @param[in] list CC_SList from which the last element is being removed
 * @param[out] out Pointer to where the removed value is stored, or NULL if it is
 *                 to be ignored
 *
 * @return CC_OK if the element was successfully removed, or CC_ERR_VALUE_NOT_FOUND
 * if the list is empty.
 */
enum cc_stat cc_slist_remove_last(CC_SList *list, void **out) {
    if (list->size == 0)
        return CC_ERR_VALUE_NOT_FOUND;

    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list, list->size - 1, &node, &prev);

    if (status != CC_OK)
        return status;

    void *e = unlinkn(list, node, prev);

    if (out)
        *out = e;

    return CC_OK;
}

/**
 * Removes all elements from the specified list.
 *
 * @param[in] list CC_SList from which all elements are being removed
 *
 * @return CC_OK if the elements were successfully removed, or CC_ERR_VALUE_NOT_FOUND
 * if the list was already empty.
 */
enum cc_stat cc_slist_remove_all(CC_SList *list) {
    bool unlinked = unlinkn_all(list, NULL);

    if (unlinked) {
        list->head = NULL;
        list->tail = NULL;
        return CC_OK;
    }
    return CC_ERR_VALUE_NOT_FOUND;
}

/**
 * Removes and frees all the elements from the specified list.
 *
 * @note
 * This function should not be called on a list that has some of it's elements
 * allocated on the stack.
 *
 * @param[in] list CC_SList from which all the elements are being removed and freed
 *
 * @return CC_OK if the element were successfully removed and freed, or
 * CC_ERR_VALUE_NOT_FOUND if the list was already empty.
 */
enum cc_stat cc_slist_remove_all_cb(CC_SList *list, void (*cb)(void *)) {
    bool unlinked = unlinkn_all(list, cb);

    if (unlinked) {
        list->head = NULL;
        list->tail = NULL;
        return CC_OK;
    }
    return CC_ERR_VALUE_NOT_FOUND;
}

/**
 * Replaces an element at the specified location and optionally sets the out parameter
 * to the value of the replaced element. The specified index must be within the bounds
 * of the list.
 *
 * @param[in] list    CC_SList on which this operation is performed
 * @param[in] element the replacement element
 * @param[in] index   index of the element being replaced
 * @param[out] out    Pointer to where the replaced element is stored, or NULL if
 *                    it is to be ignored
 *
 * @return CC_OK if the element was successfully replaced, or CC_ERR_OUT_OF_RANGE
 *         if the index was out of range.
 */
enum cc_stat cc_slist_replace_at(CC_SList *list, void *element, size_t index, void **out) {
    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list, index, &node, &prev);

    if (status != CC_OK)
        return status;

    void *old = node->data;
    node->data = element;

    if (out)
        *out = old;

    return CC_OK;
}

/**
 * Gets the first element from the specified list and sets the out parameter to
 * its value.
 *
 * @param[in] list CC_SList whose first element is being returned
 * @param[in] out  Pointer to where the element is stored
 *
 * @return CC_OK if the element was found, or CC_ERR_VALUE_NOT_FOUND if not.
 */
enum cc_stat cc_slist_get_first(CC_SList *list, void **out) {
    if (list->size == 0)
        return CC_ERR_VALUE_NOT_FOUND;

    *out = list->head->data;

    return CC_OK;
}

/**
 * Gets the last element from the specified list and sets the out parameter to
 * its value.
 *
 * @param[in] list CC_SList whose last element is being returned
 * @param[out] out Pointer to where the element is stored
 *
 * @return CC_OK if the element was found, or CC_ERR_VALUE_NOT_FOUND if not.
 */
enum cc_stat cc_slist_get_last(CC_SList *list, void **out) {
    if (list->size == 0)
        return CC_ERR_VALUE_NOT_FOUND;

    *out = list->tail->data;

    return CC_OK;
}

/**
 * Gets the list element from the specified index and sets the out parameter to
 * its value.
 *
 * @param[in] list  CC_SList from which the element is being returned.
 * @param[in] index The index of a list element being returned. The index must
 *                  be within the bound of the list.
 * @param[out] out  Pointer to where the element is stored
 *
 * @return CC_OK if the element was found, or CC_ERR_OUT_OF_RANGE if the index
 * was out of range.
 */
enum cc_stat cc_slist_get_at(CC_SList *list, size_t index, void **out) {
    SNode *prev = NULL;
    SNode *node = NULL;

    enum cc_stat status = get_node_at(list, index, &node, &prev);

    if (status != CC_OK)
        return status;

    *out = node->data;

    return CC_OK;
}

/**
 * Returns the number of elements in the specified CC_SList.
 *
 * @param[in] list CC_SList whose size is being returned
 *
 * @return The number of elements contained in the specified CC_SList.
 */
size_t cc_slist_size(CC_SList *list) {
    return list->size;
}

/**
 * Reverses the order of elements in the specified list.
 *
 * @param[in] list CC_SList that is being reversed
 */
void cc_slist_reverse(CC_SList *list) {
    if (list->size == 0 || list->size == 1)
        return;

    SNode *prev = NULL;
    SNode *next = NULL;
    SNode *flip = list->head;

    list->tail = list->head;

    while (flip) {
        next = flip->next;
        flip->next = prev;
        prev = flip;
        flip = next;
    }
    list->head = prev;
}

/**
 * Creates a sublist of the specified list. The created sublist contains all
 * the elements from the list that are contained between the two indices
 * including the elements at the indices. For example if a list contains 5
 * elements [5, 6, 7, 8, 9], a sublist from index 1 to 3 will will be a new
 * list of length 3, containing [6, 7, 8]. The created sublist is only a copy of
 * the original lists structure, meaning the data it points to is not copied.
 *
 * @note The sublist is allocated using the original lists allocators and also
 *       inherits the configuration of the original list.
 *
 * @param[in] list CC_SList from which the sublist is taken
 * @param[in] from The beginning index, ie., the first element to be included.
 *                 Must be a positive integer and may not exceed the list size
 *                 or the end index.
 * @param[in] to   The ending index, ie., the last element to be included. Must
 *                 be a positive integer no greater that the list size and no
 *                 smaller that the beginning index.
 * @param[out] out Pointer to where the new sublist is stored.
 *
 * @return CC_OK if the sublist was successfully created, CC_ERR_INVALID_RANGE
 * if the specified index range is invalid, or CC_ERR_ALLOC if the memory allocation
 * for the new sublist failed.
 */
enum cc_stat cc_slist_sublist(CC_SList *list, size_t from, size_t to, CC_SList **out) {
    if (from > to || to >= list->size)
        return CC_ERR_INVALID_RANGE;

    SNode *base = NULL;
    SNode *node = NULL;

    CC_SList *sub;
    enum cc_stat status = cc_slist_new(&sub);

    if (status != CC_OK)
        return status;

    status = get_node_at(list, from, &node, &base);

    if (status != CC_OK) {
        cc_slist_destroy(sub);
        return status;
    }

    size_t i;
    for (i = from; i <= to; i++) {
        status = cc_slist_add(sub, node->data);
        if (status != CC_OK) {
            cc_slist_destroy(sub);
            return status;
        }
        node = node->next;
    }
    *out = sub;
    return CC_OK;
}

/**
 * Creates a shallow copy of the specified list. A shallow copy is a copy of the
 * list structure. This operation does not copy the actual data that this list
 * holds.
 *
 * @note The new list is allocated using the original lists allocators and also
 *       inherits the configuration of the original list.
 *
 * @param[in] list CC_SList to be copied
 * @param[out] out Pointer to where the newly created copy is stored
 *
 * @return CC_OK if the copy was successfully created, or CC_ERR_ALLOC if the
 * memory allocation for the copy failed.
 */
enum cc_stat cc_slist_copy_shallow(CC_SList *list, CC_SList **out) {
    CC_SList *copy;
    enum cc_stat status = cc_slist_new(&copy);

    if (status != CC_OK)
        return status;

    SNode *node = list->head;

    while (node) {
        status = cc_slist_add(copy, node->data);
        if (status != CC_OK) {
            cc_slist_destroy(copy);
            return status;
        }
        node = node->next;
    }
    *out = copy;
    return CC_OK;
}

/**
 * Creates a deep copy of the specified list. This function copies the structure
 * of the list along with all the data it holds. The element copying is done
 * through the specified copy function that should return a pointer to the copy
 * of the element passed to it.
 *
 * @note The new list is allocated using the original lists allocators and also
 *       inherits the configuration of the original list.
 *
 * @param[in] list CC_SList to be copied
 * @param[in] cp   the copy function that should return a pointer to the copy of
 *                 the data.
 * @param[out] out Pointer to where the newly created copy is stored
 *
 * @return CC_OK if the copy was successfully created, or CC_ERR_ALLOC if the
 * memory allocation for the copy failed.
 */
enum cc_stat cc_slist_copy_deep(CC_SList *list, void *(*cp)(void *), CC_SList **out) {
    CC_SList *copy;
    enum cc_stat status = cc_slist_new(&copy);

    if (status != CC_OK)
        return status;

    SNode *node = list->head;

    while (node) {
        status = cc_slist_add(copy, cp(node->data));
        if (status != CC_OK) {
            cc_slist_destroy(copy);
            return status;
        }
        node = node->next;
    }
    *out = copy;
    return CC_OK;
}

/**
 * Returns an integer representing the number of occurrences of the specified
 * element within the CC_SList.
 *
 * @param[in] list CC_SList on which the search is performed
 * @param[in] element element being searched for
 *
 * @return number of found matches
 */
size_t cc_slist_contains(CC_SList *list, void *element) {
    SNode *node = list->head;
    size_t e_count = 0;

    while (node) {
        if (node->data == element)
            e_count++;
        node = node->next;
    }
    return e_count;
}

/**
 * Returns the number of occurrences of the value pointed to by <code>element</code>
 * within the specified CC_SList.
 *
 * @param[in] list CC_SList on which the search is performed
 * @param[in] element element being searched for
 * @param[in] cmp Comparator function which returns 0 if the values passed to it are equal
 *
 * @return number of occurrences of the value
 */
size_t cc_slist_contains_value(CC_SList *list, void *element, int (*cmp)(const void *, const void *)) {
    SNode *node = list->head;
    size_t e_count = 0;

    while (node) {
        if (cmp(node->data, element) == 0)
            e_count++;
        node = node->next;
    }
    return e_count;
}

/**
 * Gets the index of the specified element. The returned index is the index
 * of the first occurrence of the element starting from the beginning of the list.
 *
 * @param[in] list    the CC_SList on which this operation is performed
 * @param[in] element the element whose index is being looked up
 * @param[out] index  Pointer to where the index is stored
 *
 * @return CC_OK if the index was found, or CC_OUT_OF_RANGE if not.
 */
enum cc_stat cc_slist_index_of(CC_SList *list, void *element, size_t *index) {
    SNode *node = list->head;

    size_t i = 0;
    while (node) {
        if (node->data == element) {
            *index = i;
            return CC_OK;
        }
        i++;
        node = node->next;
    }
    return CC_ERR_OUT_OF_RANGE;
}

/**
 * Creates an array representation of the specified list. None of the elements
 * are copied into the array and thus any modification of the elements within
 * the array will affect the list elements as well. The size of the created
 * array is the same as the size of the list from which the array was constructed.
 *
 * @param[in] list CC_SList on which this operation is being performed
 * @param[out] out Pointer to where the newly created array is stored
 *
 * @return CC_OK if the array was successfully created, or CC_ERR_ALLOC if the
 * memory allocation for the new array failed.
 */
enum cc_stat cc_slist_to_array(CC_SList *list, void ***out) {
    void **array = (void **)list->mem_alloc(list->size * sizeof(void *));

    if (!array)
        return CC_ERR_ALLOC;

    SNode *node = list->head;

    size_t i;
    for (i = 0; i < list->size; i++) {
        array[i] = node->data;
        node = node->next;
    }
    *out = array;
    return CC_OK;
}

/**
 * Sorts the specified list. This function makes no guaranties that the
 * sort will be performed in place or in a stable way.
 *
 * @note
 * Pointers passed to the comparator function will be pointers to the list
 * elements that are of type (void*) ie. void**. So an extra step of
 * dereferencing will be required before the data can be used for comparison:
 * eg. <code>my_type e = *(*((my_type**) ptr));</code>.
 *
 * @param[in] list CC_SList to be sorted
 * @param[in] cmp the comparator function that must be of type <code>
 *                int cmp(const void e1*, const void e2*)</code> that
 *                returns < 0 if the first element goes before the second,
 *                0 if the elements are equal and > 0 if the second goes
 *                before the first.
 *
 * @return CC_OK if the sort was performed successfully, or CC_ERR_ALLOC
 * if the sort could not allocate enough memory to performed the sort.
 */
enum cc_stat cc_slist_sort(CC_SList *list, int (*cmp)(void const *e1, void const *e2)) {
    if (list->size == 1)
        return CC_OK;

    void **elements;
    enum cc_stat status = cc_slist_to_array(list, &elements);

    if (status != CC_OK)
        return status;

    SNode *node = list->head;

    qsort(elements, list->size, sizeof(void *), cmp);

    size_t i;
    for (i = 0; i < list->size; i++) {
        node->data = elements[i];
        node = node->next;
    }
    list->mem_free(elements);
    return CC_OK;
}

/**
 * A 'foreach loop' function that invokes the specified function on each element
 * in the list.
 *
 * @param[in] list the list on which this operation is being performed
 * @param[in] op the operation function that is to be invoked on each list
 *               element
 */
void cc_slist_foreach(CC_SList *list, void (*op)(void *)) {
    SNode *n = list->head;

    while (n) {
        op(n->data);
        n = n->next;
    }
}

/**
 * Filters the CC_SList by creating a new CC_SList that contains all elements from the
 * original CC_SList that return true on pred(element) without modifying the original
 * CC_SList.
 *
 * @param[in] list Slist that is to be filtered
 * @param[in] pred predicate function which returns true if the element should
 *                 be kept in the filtered CC_SList
 * @param[out] out pointer to where the new filtered CC_SList is to be stored
 *
 * @return CC_OK if the CC_SList was filtered successfully, CC_ERR_OUT_OF_RANGE
 * if the CC_SList is empty, or CC_ERR_ALLOC if the memory allocation for the
 * new CC_SList failed.
 */
enum cc_stat cc_slist_filter(CC_SList *list, bool (*pred)(const void *), CC_SList **out) {
    if (cc_slist_size(list) == 0)
        return CC_ERR_OUT_OF_RANGE;

    CC_SList *filtered = NULL;
    cc_slist_new(&filtered);

    if (!filtered)
        return CC_ERR_ALLOC;

    SNode *curr = list->head;
    while (curr) {
        if (pred(curr->data)) {
            cc_slist_add(filtered, curr->data);
        }

        curr = curr->next;
    }

    *out = filtered;
    return CC_OK;
}

/**
 * Filters the CC_SList by modifying it. It removes all elements that don't
 * return true on pred(element).
 *
 * @param[in] list Slist that is to be filtered
 * @param[in] pred predicate function which returns true if the element should
 *                 be kept in the Slist
 *
 * @return CC_OK if the Slist was filtered successfully, or CC_ERR_OUT_OF_RANGE
 * if the Slist is empty.
 */
enum cc_stat cc_slist_filter_mut(CC_SList *list, bool (*pred)(const void *)) {
    if (cc_slist_size(list) == 0)
        return CC_ERR_OUT_OF_RANGE;

    SNode *curr = list->head;
    SNode *next = NULL, *prev = NULL;

    while (curr) {
        next = curr->next;

        if (!pred(curr->data)) {
            unlinkn(list, curr, prev);
        } else {
            prev = curr;
        }
        curr = next;
    }

    return CC_OK;
}

/**
 * Initializes the iterator.
 *
 * @param[in] iter the iterator that is being initialized
 * @param[in] list CC_SList to iterate over
 */
void cc_slist_iter_init(CC_SListIter *iter, CC_SList *list) {
    iter->index = 0;
    iter->list = list;
    iter->current = NULL;
    iter->prev = NULL;
    iter->next = list->head;
}

/**
 * Removes the last returned element by <code>cc_slist_iter_next()</code>
 * function without invalidating the iterator and optionally sets the out
 * parameter to the value of the removed element.
 *
 * @note This function should only ever be called after a call to <code>
 * cc_slist_iter_next()</code>
 *
 * @param[in] iter the iterator on which this operation is being performed
 * @param[out] out Pointer to where the removed element is stored, or NULL
 *                 if it is to be ignored
 *
 * @return CC_OK if the element was successfully removed, or
 * CC_ERR_VALUE_NOT_FOUND
 */
enum cc_stat cc_slist_iter_remove(CC_SListIter *iter, void **out) {
    if (!iter->current)
        return CC_ERR_VALUE_NOT_FOUND;

    void *e = unlinkn(iter->list, iter->current, iter->prev);
    iter->current = NULL;
    iter->index--;

    if (out)
        *out = e;

    return CC_OK;
}

/**
 * Adds a new element to the list after the last returned element by
 * <code>cc_slist_iter_next()</code> function without invalidating the
 * iterator.
 *
 * @note This function should only ever be called after a call to <code>
 * cc_slist_iter_next()</code>
 *
 * @param[in] iter the iterator on which this operation is being performed
 * @param[in] element the element being added
 *
 * @return CC_OK if the element was successfully added, or CC_ERR_ALLOC
 * if the memory allocation for the new element failed.
 */
enum cc_stat cc_slist_iter_add(CC_SListIter *iter, void *element) {
    SNode *new_node = (SNode *)iter->list->mem_calloc(1, sizeof(SNode));

    if (!new_node)
        return CC_ERR_ALLOC;

    new_node->data = element;
    new_node->next = iter->next;

    iter->current->next = new_node;

    if (iter->index == iter->list->size)
        iter->list->tail = new_node;

    iter->index++;
    iter->list->size++;
    return CC_OK;
}

/**
 * Replaces the last returned element by <code>cc_slist_iter_next()</code>
 * with the specified element and optionally sets the out parameter to
 * the value of the replaced element.
 *
 * @note This function should only ever be called after a call to <code>
 * cc_slist_iter_next()</code>

 * @param[in] iter the iterator on which this operation is being preformed
 * @param[in] element the replacement element
 * @param[out] out Pointer to where the replaced element is stored, or NULL
 *                if it is to be ignored
 *
 * @return CC_OK if the element was replaced successfully, or
 * CC_ERR_VALUE_NOT_FOUND
 */
enum cc_stat cc_slist_iter_replace(CC_SListIter *iter, void *element, void **out) {
    if (!iter->current)
        return CC_ERR_VALUE_NOT_FOUND;

    void *old = iter->current->data;
    iter->current->data = element;

    if (out)
        *out = old;

    return CC_OK;
}

/**
 * Advances the iterator and sets the out parameter to the value of the
 * next element in the sequence.
 *
 * @param[in] iter the iterator that is being advanced
 * @param[out] out Pointer to where the next element is set
 *
 * @return CC_OK if the iterator was advanced, or CC_ITER_END if the
 * end of the list has been reached.
 */
enum cc_stat cc_slist_iter_next(CC_SListIter *iter, void **out) {
    if (!iter->next)
        return CC_ITER_END;

    void *data = iter->next->data;

    if (iter->current)
        iter->prev = iter->current;

    iter->current = iter->next;
    iter->next = iter->next->next;
    iter->index++;

    *out = data;

    return CC_OK;
}

/**
 * Returns the index of the last returned element by <code>cc_slist_iter_next()
 * </code>.
 *
 * @param[in] iter the iterator on which this operation is being performed
 *
 * @return the index
 */
size_t cc_slist_iter_index(CC_SListIter *iter) {
    return iter->index - 1;
}

/**
 * Initializes the zip iterator.
 *
 * @param[in] iter Iterator that is being initialized
 * @param[in] l1   First CC_SList
 * @param[in] l2   Second CC_SList
 */
void cc_slist_zip_iter_init(CC_SListZipIter *iter, CC_SList *l1, CC_SList *l2) {
    iter->index = 0;
    iter->l1 = l1;
    iter->l2 = l2;
    iter->l1_current = NULL;
    iter->l2_current = NULL;
    iter->l1_prev = NULL;
    iter->l2_prev = NULL;
    iter->l1_next = l1->head;
    iter->l2_next = l2->head;
}

/**
 * Outputs the next element pair in the sequence and advances the iterator.
 *
 * @param[in]  iter Iterator that is being advanced
 * @param[out] out1 Output of the first CC_SList element
 * @param[out] out2 Output of the second CC_SList element
 *
 * @return CC_OK if a next element pair is returned, or CC_ITER_END if the end of one
 * of the lists has been reached.
 */
enum cc_stat cc_slist_zip_iter_next(CC_SListZipIter *iter, void **out1, void **out2) {
    if (!iter->l1_next || !iter->l2_next)
        return CC_ITER_END;

    void *data1 = iter->l1_next->data;
    void *data2 = iter->l2_next->data;

    if (iter->l1_current)
        iter->l1_prev = iter->l1_current;

    if (iter->l2_current)
        iter->l2_prev = iter->l2_current;

    iter->l1_current = iter->l1_next;
    iter->l2_current = iter->l2_next;
    iter->l1_next = iter->l1_next->next;
    iter->l2_next = iter->l2_next->next;

    iter->index++;

    *out1 = data1;
    *out2 = data2;

    return CC_OK;
}

/**
 * Adds a new element pair to the slists after the last returned element pair by
 * <code>cc_slist_zip_iter_next()</code> and immediately before an element pair
 * that would be returned by a subsequent call to <code>cc_slist_zip_iter_next()</code>
 * without invalidating the iterator.
 *
 * @note This function should only ever be called after a call to <code>
 * cc_slist_zip_iter_next()</code>
 *
 * @param[in] iter Iterator on which this operation is being performed
 * @param[in] e1   element added to the first slist
 * @param[in] e2   element added to the second slist
 *
 * @return CC_OK if the element pair was successfully added to the slists, or
 * CC_ERR_ALLOC if the memory allocation for the new elements failed.
 */
enum cc_stat cc_slist_zip_iter_add(CC_SListZipIter *iter, void *e1, void *e2) {
    SNode *new_node1 = (SNode *)iter->l1->mem_calloc(1, sizeof(SNode));

    if (!new_node1)
        return CC_ERR_ALLOC;

    SNode *new_node2 = (SNode *)iter->l2->mem_calloc(1, sizeof(SNode));

    if (!new_node2) {
        iter->l1->mem_free(new_node1);
        return CC_ERR_ALLOC;
    }

    new_node1->data = e1;
    new_node2->data = e2;

    new_node1->next = iter->l1_next;
    new_node2->next = iter->l2_next;

    iter->l1_current->next = new_node1;
    iter->l2_current->next = new_node2;

    if (iter->index == iter->l1->size)
        iter->l1->tail = new_node1;

    if (iter->index == iter->l2->size)
        iter->l2->tail = new_node2;

    iter->index++;
    iter->l1->size++;
    iter->l2->size++;

    return CC_OK;
}

/**
 * Removes and outputs the last returned element pair by <code>cc_slist_zip_iter_next()
 * </code> without invalidating the iterator.
 *
 * @note This function should only ever be called after a call to <code>
 * cc_slist_zip_iter_next()</code>
 *
 * @param[in]  iter Iterator on which this operation is being performed
 * @param[out] out1 Output of the removed element from the first Slist
 * @param[out] out2 Output of the removed element from the second Slist
 *
 * @return CC_OK of the element was successfully removed, or CC_ERR_VALUE_NOT_FOUND.
 */
enum cc_stat cc_slist_zip_iter_remove(CC_SListZipIter *iter, void **out1, void **out2) {
    if (!iter->l1_current || !iter->l2_current)
        return CC_ERR_VALUE_NOT_FOUND;

    void *e1 = unlinkn(iter->l1, iter->l1_current, iter->l1_prev);
    void *e2 = unlinkn(iter->l2, iter->l2_current, iter->l2_prev);

    iter->l1_current = NULL;
    iter->l2_current = NULL;

    iter->index--;

    if (out1)
        *out1 = e1;

    if (out2)
        *out2 = e2;

    return CC_OK;
}

/**
 * Replaces the last returned element pair by <code>cc_slist_zip_iter_next()</code>
 * with the specified replacement element pair.
 *
 * @note This function should only ever be called after a call to <code>
 * cc_slist_zip_iter_next()</code>
 *
 * @param[in]  iter Iterator on which this operation is being performed
 * @param[in]  e1   First slist's replacement element
 * @param[in]  e2   Second slist's replacement element
 * @param[out] out1 Output of the replaced element from the first slist
 * @param[out] out2 Output of the replaced element from the second slist
 *
 * @return CC_OK of the element was successfully replaced, or CC_ERR_VALUE_NOT_FOUND.
 */
enum cc_stat cc_slist_zip_iter_replace(CC_SListZipIter *iter, void *e1, void *e2, void **out1, void **out2) {
    if (!iter->l1_current || !iter->l2_current)
        return CC_ERR_VALUE_NOT_FOUND;

    void *old1 = iter->l1_current->data;
    void *old2 = iter->l2_current->data;

    iter->l1_current->data = e1;
    iter->l2_current->data = e2;

    if (out1)
        *out1 = old1;

    if (out2)
        *out2 = old2;

    return CC_OK;
}

/**
 * Returns the index of the last returned element pair by <code>cc_slist_zip_iter_next()</code>.
 *
 * @param[in] iter Iterator on which this operation is being performed
 *
 * @return current iterator index
 */
size_t cc_slist_zip_iter_index(CC_SListZipIter *iter) {
    return iter->index - 1;
}

/**
 * Unlinks the node from the list and returns the data that was associated with it and
 * also adjusts the head / tail of the list if necessary.
 *
 * @param[in] list the list from which the node is being unlinked
 * @parma[in] node the node being unlinked
 * @param[in] prev the node that immediately precedes the node that is being unlinked
 *
 * @return the data that was at this node
 */
static void *unlinkn(CC_SList *list, SNode *node, SNode *prev) {
    void *data = node->data;

    if (prev)
        prev->next = node->next;
    else
        list->head = node->next;

    if (!node->next)
        list->tail = prev;

    list->mem_free(node);
    list->size--;

    return data;
}

/**
 * Unlinks all nodes from the list and optionally frees the data at the nodes.
 *
 * @param[in] list the list from which all nodes are being unlinked
 * @param[in] freed specified whether or not the data at the nodes should also
 *                  be deallocated.
 *
 * @return false if the list is already y empty, otherwise returns true
 */
static bool unlinkn_all(CC_SList *list, void (*cb)(void *)) {
    if (list->size == 0)
        return false;

    SNode *n = list->head;

    while (n) {
        SNode *tmp = n->next;

        if (cb)
            cb(n->data);

        list->mem_free(n);
        n = tmp;
        list->size--;
    }
    return true;
}

/**
 * Finds the node at the specified index. If the index is not in the bounds
 * of the list, NULL is returned instead.
 *
 * @param[in] list the list from which the node is being returned
 * @param[in] index the index of the node
 * @param[out] node the node at the specified index
 * @param[out] prev the node that immediately precedes the node at the
 *                  specified index
 *
 * @return CC_OK if the node was found, or CC_ERR_OUT_OF_RANGE if the index
 * was out of range.
 */
static enum cc_stat
get_node_at(CC_SList *list, size_t index, SNode **node, SNode **prev) {
    if (index >= list->size)
        return CC_ERR_OUT_OF_RANGE;

    *node = list->head;
    *prev = NULL;

    size_t i;
    for (i = 0; i < index; i++) {
        *prev = *node;
        *node = (*node)->next;
    }
    return CC_OK;
}

/**
 * Finds the first node from the beginning of the list that is associated
 * with the specified element. If no node is associated with the element,
 * NULL is returned instead.
 *
 * @param[in] list the list from which the node is being returned
 * @param[in] element the element whose list node is being returned
 * @param[out] node the node associated with the data
 * @param[out] prev the node that immediately precedes the node at the
 *                  specified index
 *
 * @return CC_OK if the node was found, or CC_ERR_VALUE_NOT_FOUND if not.
 */
static enum cc_stat
get_node(CC_SList *list, void *element, SNode **node, SNode **prev) {
    *node = list->head;
    *prev = NULL;

    while (*node) {
        if ((*node)->data == element)
            return CC_OK;

        *prev = *node;
        *node = (*node)->next;
    }
    return CC_ERR_VALUE_NOT_FOUND;
}

size_t cc_slist_struct_size() {
    return sizeof(CC_SList);
}

#ifdef __cplusplus
}
#endif

#endif

#include "../include/datastructure.h"

// List
List::List() {
    head = new ListNode();
    tail = head;
    listsize = 0;
}

List::~List() {
    for (ListNode *p = head, *q = head->next; q != nullptr; p = q, q = q->next)
        free(p);
    free(tail);
}

void List::L_push(DataType data, int pos) {
    assert(pos >= 0 || pos <= listsize);
    ListNode *new_node = new ListNode(data);
    if (pos == listsize) {
        tail->next = new_node;
        new_node->prev = tail;
        tail = new_node;
    } else {
        ListNode *p = head;
        for (int i = 0; i < pos; i++) p = p->next;
        new_node->next = p->next;
        new_node->prev = p;
        p->next->prev = new_node;
        p->next = new_node;
    }
    listsize++;
}

void List::L_push_front(DataType data) { L_push(data, 0); }

void List::L_push_back(DataType data) { L_push(data, listsize); }

void List::L_pop_front() {
    assert(head->next != nullptr);
    ListNode *p = head->next;
    head->next = p->next;
    if (p->next != nullptr)
    p->next->prev = head;
    if (p == tail) tail = head;
    free(p);
    listsize--;
}

void List::L_pop_back() {
    assert(tail != head);
    ListNode *p = tail->prev, *q = tail;
    free(q);
    tail = p;
    tail->next = nullptr;
    listsize--;
}

DataType List::L_front() {
    assert(head->next != nullptr);
    return head->next->data;
}

DataType List::L_back() {
    assert(tail != head);
    return tail->data;
}

void List::L_erase(ListNode *node) {
    ListNode *p = node->prev;
    if (node == tail) tail = p;
    p->next = node->next;
    if (node->next != nullptr) node->next->prev = p;
    free(node);
    listsize--;
}

int List::L_size() { return listsize; }

bool List::L_isEmpty() { return (head->next == nullptr); }

int List::L_distance(ListNode *start, ListNode *end) {
    assert(start != nullptr && end != nullptr);
    int i;
    for (i = 0; start != end && start != nullptr; i++) start = start->next;
    assert(start != nullptr);
    return i;
}

ListNode * List::L_advance(ListNode *it, int len) {
    for (int i = 0; i < len; i++) {
        assert(it != nullptr);
        it = it->next;
    }
    return it;
}

ListNode *List::L_begin() { return head->next; }

ListNode *List::L_end() { return tail; }

// HashTable
HashTable::HashTable() {
    for (int i = 0; i < tablesize; i++) {
        hashtable[i] = new HashItem();
        hashtable[i]->key = -1;
        hashtable[i]->value = nullptr;
        hashtable[i]->next = nullptr;
    }
}

HashTable::~HashTable() {}

int HashTable::HashFuction(KeyType key) { return key % tablesize; }

void HashTable::H_modValue(KeyType key, ValueType value) {}

void HashTable::H_insert(KeyType key, ValueType value) {
    HashItem *p = H_find(key);
    if (p != nullptr) {
        p->value = value;
    } else {
        int index = HashFuction(key);
        if (hashtable[index]->key == -1) {
            hashtable[index]->key = key;
            hashtable[index]->value = value;
            hashtable[index]->next = nullptr;
        } else {
            HashItem *q = hashtable[index];
            HashItem *new_item = new HashItem(key, value);
            while (q->next != nullptr) q = q->next;
            q->next = new_item;
        }
    }
}

void HashTable::H_erase(KeyType key) {
    int index = HashFuction(key);
    HashItem *dp, *p, *q;
    assert(hashtable[index]->key != -1);
    if (hashtable[index]->key == key && hashtable[index]->next == nullptr)
        hashtable[index]->key = -1;
    else if (hashtable[index]->key == key) {
        dp = hashtable[index];
        hashtable[index] = hashtable[index]->next;
        free(dp);
    } else {
        p = hashtable[index]->next;
        q = hashtable[index];
        while (p != nullptr && p->key != key) {
            q = p;
            p = p->next;
        }
        assert(p != nullptr);
        dp = p;
        p = p->next;
        q->next = p;
        free(dp);
    }
}

HashItem *HashTable::H_find(KeyType key) {
    int index = HashFuction(key);

    HashItem *p = hashtable[index];
    if (p->value == nullptr) return nullptr;
    while (p != nullptr) {
        if (p->key == key) return p;
        p = p->next;
    }
    return nullptr;
}
#include <assert.h>
#include <stdlib.h>

// define List
typedef int DataType;

typedef struct ListNode {
    DataType data;
    ListNode *next;

    ListNode() : data(-1), next(nullptr) {}
    ListNode(DataType d) : data(d), next(nullptr) {}
} ListNode;

class List {
   public:
    List();

    ~List();

    void L_push(DataType data, int pos);

    void L_push_front(DataType data);

    void L_push_back(DataType data);

    void L_pop_front();

    void L_pop_back();

    DataType L_front();

    DataType L_back();

    void L_erase(ListNode *node);

    int L_size();

    bool L_isEmpty();

    int L_distance(ListNode *start, ListNode *end);

    ListNode * L_advance(ListNode *it, int len);

    ListNode *L_begin();

    ListNode *L_end();

   private:
    ListNode *head;
    ListNode *tail;
    int listsize;
};

// define Hash Table
typedef int KeyType;
typedef ListNode *ValueType;

typedef struct HashItem {
    KeyType key;
    ValueType value;
    HashItem *next;

    HashItem() : key(-1), value(nullptr), next(nullptr) {}
    
    HashItem(KeyType k, ValueType v) : key(k), value(v), next(nullptr) {}
} HashItem;

class HashTable {
   private:
    static const int tablesize = 1000;
    HashItem *hashtable[tablesize];

   public:
    HashTable();

    ~HashTable();

    int HashFuction(KeyType key);

    void H_modValue(KeyType key, ValueType value);

    void H_insert(KeyType key, ValueType value);

    void H_erase(KeyType key);

    HashItem *H_find(KeyType key);
};

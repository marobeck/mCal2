#ifndef DLL_H
#define DLL_H

#include <stddef.h>

// Define the node structure
typedef struct Node
{
    void *data;
    struct Node *next;
    struct Node *prev;
} Node;

class DoublyLinkedList
{
private:
    Node *head;  // First element in list
    Node *tail;  // Last element in list
    size_t size; // Size of list

public:
    /**
     * Creates an empty linked list
     */
    DoublyLinkedList() : size(0), head(NULL), tail(NULL) {}

    ~DoublyLinkedList();

    /* ----------------------------- Appending data ----------------------------- */

    /**
     * @brief Inserts data into a node at the front of the list
     * @param data Data to add
     */
    void insert_front(void *data);

    /**
     * @brief Inserts data into a node at the front of the list
     * @param data Data to add
     */
    void insert_back(void *data);

    /* ------------------------------ Editing data ------------------------------ */

    /**
     * @brief Delete a node from the list given its address
     * @param list The list in question
     * @param node The node to remove (use find_node)
     */
    void delete_node(Node *node);

    /* ----------------------------- Searching list ----------------------------- */

    /**
     * @brief Finds the pointer of a node given it's target data, returning the first hit
     * @param data Data to search for
     * @param cmp Function to compare data with
     * @return Pointer to the first (head->tail) node holding the desired data
     */
    Node *find_node(void *data, int (*cmp)(void *, void *));

    /**
     * @brief Traverses from head to tail and parses each value through the given function
     * @param func Function to call
     */
    void traverse_forward(void (*func)(void *));

    /**
     * @brief Traverses from tail to head and parses each value through the given function
     * @param func Function to call
     */
    void traverse_backward(void (*func)(void *));
};

#endif // DLL_H

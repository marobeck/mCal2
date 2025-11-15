#include <stdlib.h>
#include <stdexcept>
#include "DLL.h"

DoublyLinkedList::~DoublyLinkedList()
{
    Node *current = head;
    while (current)
    {
        Node *next = current->next;
        if (current->data)
        {
            delete current->data; // Delete node contents
        }
        free(current);  // Delete node
        current = next; // Traverse to next node
    }
}

void DoublyLinkedList::insert_front(void *data)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node)
        throw std::runtime_error("malloc failed");
    new_node->data = data;
    new_node->next = head;
    new_node->prev = NULL;
    if (head)
    {
        head->prev = new_node;
    }
    else
    {
        tail = new_node;
    }
    head = new_node;
    size++;
}

/// @brief Insert a node at the back of the list
/// @param list The list in question
/// @param data Data to add
/// @return 0 on success, 1 if inputs are invalid
void DoublyLinkedList::insert_back(void *data)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node)
        throw std::runtime_error("malloc failed");
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = tail;
    if (tail)
    {
        tail->next = new_node;
    }
    else
    {
        head = new_node;
    }
    tail = new_node;
    size++;
}

void DoublyLinkedList::delete_node(Node *node)
{
    if (!node)
    {
        throw std::invalid_argument("Invalid node");
    }

    // Close gap
    if (node->prev)
    {
        node->prev->next = node->next;
    }
    else
    {
        head = node->next;
    }
    if (node->next)
    {
        node->next->prev = node->prev;
    }
    else
    {
        tail = node->prev;
    }

    // Free the data
    if (node->data)
    {
        delete node->data;
    }
    free(node);
    size--;
}

// Find a node in the list
Node *DoublyLinkedList::find_node(void *data, int (*cmp)(void *, void *))
{
    Node *current = head;
    while (current)
    {
        if (cmp(current->data, data) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Traverse the list forward
void DoublyLinkedList::traverse_forward(void (*func)(void *))
{
    Node *current = head;
    while (current)
    {
        func(current->data);
        current = current->next;
    }
}

// Traverse the list backward
void DoublyLinkedList::traverse_backward(void (*func)(void *))
{
    Node *current = tail;
    while (current)
    {
        func(current->data);
        current = current->prev;
    }
}
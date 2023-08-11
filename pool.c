#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pool.h"

void initializeLetterPool(struct LetterPool* pool) {
    pool->head = NULL;
    pool->totalWeight = 0;
}

void addLetter(struct LetterPool* pool, char letter, int weight) {
    struct LetterNode* newNode = (struct LetterNode*)malloc(sizeof(struct LetterNode));
    newNode->letter = letter;
    newNode->weight = weight;
    newNode->next = NULL;

    if (pool->head == NULL) {
        pool->head = newNode;
    } else {
        struct LetterNode* current = pool->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }

    pool->totalWeight += weight;
}

char getRandomLetter(struct LetterPool* pool) {
    int randomWeight = rand() % pool->totalWeight;
    struct LetterNode* current = pool->head;

    while (current != NULL) {
        randomWeight -= current->weight;
        if (randomWeight < 0) {
            return current->letter;
        }
        current = current->next;
    }

    // This should not be reached under normal circumstances
    return '\0';
}

void destroyLetterPool(struct LetterPool* pool) {
    struct LetterNode* current = pool->head;
    while (current != NULL) {
        struct LetterNode* next = current->next;
        free(current);
        current = next;
    }
    pool->head = NULL;
    pool->totalWeight = 0;
}

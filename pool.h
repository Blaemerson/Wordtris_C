#pragma once

struct LetterNode {
    char letter;
    int weight;
    struct LetterNode* next;
};

struct LetterPool {
    struct LetterNode* head;
    int totalWeight;
};

void initializeLetterPool(struct LetterPool* pool);

void addLetter(struct LetterPool* pool, char letter, int weight);

char getRandomLetter(struct LetterPool* pool);

void destroyLetterPool(struct LetterPool* pool);

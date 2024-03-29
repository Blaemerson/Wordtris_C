#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"

struct TrieNode* createNode() {
    struct TrieNode* node = (struct TrieNode*)malloc(sizeof(struct TrieNode));

    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }

    node->isEndOfWord = false;

    return node;
}

void destroyTrie(struct TrieNode* node) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < 26; i++) {
        destroyTrie(node->children[i]);
    }

    free(node);
}

void insertWord(struct TrieNode* root, const char* word) {
    struct TrieNode* curr = root;

    for (int i = 0; word[i] != '\0'; i++) {
        int index = word[i] - 'a'; // Convert character to index (assuming only lowercase alphabets)

        if (curr->children[index] == NULL) {
            curr->children[index] = createNode();
        }

        curr = curr->children[index];
    }

    curr->isEndOfWord = true;
}

int constructTrie(struct TrieNode* root, const char* dictionaryFile) {
    FILE* file = fopen(dictionaryFile, "r");

    if (file == NULL) {
        // Handle error opening the dictionary file
        printf("ERROR in constructTrie: unable to open dictionary file");
        return -1;
    }

    char word[10];

    while (fgets(word, sizeof(word), file) != NULL) {
        // Remove newline character from the word, if present
        word[strcspn(word, "\n")] = '\0';

        insertWord(root, word);
    }

    fclose(file);

    return 0;
}

bool searchWord(struct TrieNode* root, const char* word) {
    struct TrieNode* curr = root;

    for (int i = 0; word[i] != '\0'; i++) {
        int index = word[i] - 'a'; // Convert character to index (assuming only lowercase alphabets)

        if (curr->children[index] == NULL) {
            return false; // Word does not exist
        }

        curr = curr->children[index];
    }

    return (curr != NULL && curr->isEndOfWord);
}


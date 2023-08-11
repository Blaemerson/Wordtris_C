#define MAX_CHILDREN 26

struct TrieNode {
    struct TrieNode* children[MAX_CHILDREN]; // Array to store child nodes for each letter (assuming only lowercase alphabets)
    bool isEndOfWord; // Flag to mark the end of a word
};

struct TrieNode* createNode();

void destroyTrie(struct TrieNode* node);

void insertWord(struct TrieNode* root, const char* word);

int constructTrie(struct TrieNode* root, const char* dictionaryFile);

bool searchWord(struct TrieNode* root, const char* word);

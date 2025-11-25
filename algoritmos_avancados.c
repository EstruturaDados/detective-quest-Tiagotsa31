#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_TAM 101

/* ======================================
   ESTRUTURAS
   ====================================== */

typedef struct Sala {
    char *nome;
    char *pista;
    struct Sala *esq;
    struct Sala *dir;
} Sala;

typedef struct PistaNode {
    char *pista;
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

typedef struct HashNode {
    char *chave_pista;
    char *suspeito;
    struct HashNode *next;
} HashNode;

typedef struct HashTable {
    HashNode *buckets[HASH_TAM];
} HashTable;

/* ======================================
   FUNÇÕES UTILITÁRIAS
   ====================================== */

static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) {
        fprintf(stderr, "Erro de memória strdup.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(p, s, n);
    return p;
}

static unsigned long hash_str(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + (unsigned long)c; // Conversão explícita
    return hash;
}

/* ======================================
   ÁRVORE DE SALAS
   ====================================== */

Sala *criarSala(const char *nome, const char *pista) {
    Sala *s = malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro malloc sala.\n");
        exit(EXIT_FAILURE);
    }
    s->nome = strdup_safe(nome);
    s->pista = pista ? strdup_safe(pista) : NULL;
    s->esq = s->dir = NULL;
    return s;
}

void liberarSalas(Sala *r) {
    if (!r) return;
    liberarSalas(r->esq);
    liberarSalas(r->dir);
    free(r->nome);
    if (r->pista) free(r->pista);
    free(r);
}

/* ======================================
   BST DE PISTAS
   ====================================== */

PistaNode *inserirPista(PistaNode *r, const char *text) {
    if (!text) return r;
    if (!r) {
        PistaNode *n = malloc(sizeof(PistaNode));
        if (!n) { fprintf(stderr, "Erro malloc pista.\n"); exit(EXIT_FAILURE); }
        n->pista = strdup_safe(text);
        n->esq = n->dir = NULL;
        return n;
    }

    int cmp = strcmp(text, r->pista);
    if (cmp == 0) return r;
    if (cmp < 0) r->esq = inserirPista(r->esq, text);
    else r->dir = inserirPista(r->dir, text);

    return r;
}

PistaNode *adicionarPista(PistaNode *r, const char *t) {
    return inserirPista(r, t);
}

void liberarPistas(PistaNode *r) {
    if (!r) return;
    liberarPistas(r->esq);
    liberarPistas(r->dir);
    free(r->pista);
    free(r);
}

void exibirPistas(PistaNode *r) {
    if (!r) return;
    exibirPistas(r->esq);
    printf("- %s\n", r->pista);
    exibirPistas(r->dir);
}

/* ======================================
   HASH
   ====================================== */

void initHash(HashTable *ht) {
    for (int i = 0; i < HASH_TAM; i++)
        ht->buckets[i] = NULL;
}

void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    unsigned long h = hash_str(pista) % HASH_TAM;

    HashNode *n = malloc(sizeof(HashNode));
    if (!n) { fprintf(stderr, "Erro malloc hash.\n"); exit(EXIT_FAILURE); }

    n->chave_pista = strdup_safe(pista);
    n->suspeito = strdup_safe(suspeito);
    n->next = ht->buckets[h];
    ht->buckets[h] = n;
}

const char *encontrarSuspeito(HashTable *ht, const char *pista) {
    unsigned long h = hash_str(pista) % HASH_TAM;
    HashNode *cur = ht->buckets[h];

    while (cur) {
        if (strcmp(cur->chave_pista, pista) == 0)
            return cur->suspeito;
        cur = cur->next;
    }
    return NULL;
}

void liberarHash(HashTable *ht) {
    for (int i = 0; i < HASH_TAM; i++) {
        HashNode *cur = ht->buckets[i];
        while (cur) {
            HashNode *nx = cur->next;
            free(cur->chave_pista);
            free(cur->suspeito);
            free(cur);
            cur = nx;
        }
    }
}

/* ======================================
   JULGAMENTO – Função auxiliar sem aninhar!
   ====================================== */

void contarPistasSuspeito(PistaNode *r, HashTable *ht, const char *acusado, int *contador) {
    if (!r) return;

    contarPistasSuspeito(r->esq, ht, acusado, contador);

    const char *s = encontrarSuspeito(ht, r->pista);
    if (s && strcmp(s, acusado) == 0)
        (*contador)++;

    contarPistasSuspeito(r->dir, ht, acusado, contador);
}

/* ======================================
   EXPLORAÇÃO
   ====================================== */

PistaNode *explorarSalas(Sala *atual) {
    if (!atual) return NULL;

    char entrada[64];
    PistaNode *bst = NULL;

    printf("===== INÍCIO DA EXPLORAÇÃO =====\n");

    while (1) {
        printf("\nVocê está em: %s\n", atual->nome);

        if (atual->pista) {
            printf("Pista encontrada: \"%s\"\n", atual->pista);
            bst = adicionarPista(bst, atual->pista);
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        printf("\n[e] esquerda | [d] direita | [s] sair\nEscolha: ");

        if (!fgets(entrada, sizeof(entrada), stdin)) break;

        char op = entrada[0];

        if (op == 's' || op == 'S') break;

        if (op == 'e' || op == 'E') {
            if (atual->esq) atual = atual->esq;
            else printf("Caminho inexistente.\n");
        }
        else if (op == 'd' || op == 'D') {
            if (atual->dir) atual = atual->dir;
            else printf("Caminho inexistente.\n");
        }
        else printf("Comando inválido.\n");
    }

    return bst;
}

/* ======================================
   VERIFICAÇÃO FINAL
   ====================================== */

int verificarSuspeitoFinal(PistaNode *pistas, HashTable *ht, const char *acusado) {
    if (!acusado || strlen(acusado) == 0) {
        printf("Nome inválido.\n");
        return 0;
    }

    int contador = 0;
    contarPistasSuspeito(pistas, ht, acusado, &contador);

    printf("\n===== JULGAMENTO =====\n");
    printf("Acusado: %s\n", acusado);
    printf("Pistas que apontam para ele: %d\n", contador);

    if (contador >= 2) {
        printf("VEREDITO: Culpado.\n");
        return 1;
    }

    printf("VEREDITO: Inocente por falta de provas.\n");
    return 0;
}

/* ======================================
   MAIN – TODAS AS ETAPAS IMPLEMENTADAS
   ====================================== */

int main() {

    /* --- Construção da mansão (árvore binária) --- */
    Sala *hall       = criarSala("Hall de Entrada", "Pegadas molhadas próximas à janela");
    Sala *estar      = criarSala("Sala de Estar",   "Colar quebrado com fios de seda");
    Sala *biblioteca = criarSala("Biblioteca",      "Página arrancada com anotações sobre dinheiro");
    Sala *cozinha    = criarSala("Cozinha",         "Resíduo químico em um copo");
    Sala *despensa   = criarSala("Despensa",        "Lata marcante com rótulo raspado");
    Sala *jardim     = criarSala("Jardim",          "Pegadas que seguem para o portão");
    Sala *corredor   = criarSala("Corredor",        "Mancha de lama recente");
    Sala *quarto     = criarSala("Quarto",          "Roupas rasgadas e um fio de cabelo loiro");
    Sala *sotao      = criarSala("Sótão",           "Chave antiga enferrujada");

    hall->esq = estar;    hall->dir = corredor;
    estar->esq = biblioteca; estar->dir = cozinha;
    cozinha->esq = despensa; cozinha->dir = jardim;
    corredor->esq = quarto;  corredor->dir = sotao;

    /* --- Tabela hash com associações pista ➝ suspeito --- */
    HashTable ht;
    initHash(&ht);

    inserirNaHash(&ht, "Pegadas molhadas próximas à janela", "Sr. Green");
    inserirNaHash(&ht, "Colar quebrado com fios de seda", "Sra. Peacock");
    inserirNaHash(&ht, "Página arrancada com anotações sobre dinheiro", "Professor Plum");
    inserirNaHash(&ht, "Resíduo químico em um copo", "Dr. Orchid");
    inserirNaHash(&ht, "Lata marcante com rótulo raspado", "Mr. Boddy");
    inserirNaHash(&ht, "Pegadas que seguem para o portão", "Sr. Green");
    inserirNaHash(&ht, "Mancha de lama recente", "Mr. Boddy");
    inserirNaHash(&ht, "Roupas rasgadas e um fio de cabelo loiro", "Sra. Peacock");
    inserirNaHash(&ht, "Chave antiga enferrujada", "Professor Plum");

    /* --- Exploração --- */
    PistaNode *coletadas = explorarSalas(hall);

    printf("\n===== PISTAS COLETADAS =====\n");
    if (!coletadas) printf("Nenhuma pista coletada.\n");
    else exibirPistas(coletadas);

    /* --- Fase de acusação --- */
    char acusado[128];
    printf("\nDigite o nome do suspeito acusado: ");
    fgets(acusado, sizeof(acusado), stdin);
    acusado[strcspn(acusado, "\n")] = '\0';

    verificarSuspeitoFinal(coletadas, &ht, acusado);

    /* --- Liberações --- */
    liberarPistas(coletadas);
    liberarSalas(hall);
    liberarHash(&ht);

    printf("\nObrigado por jogar Detective Quest!\n");
    return 0;
}
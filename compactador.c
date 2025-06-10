#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif

#define TAM 256

// Estruturas
typedef struct noArvore {
    char caractere;
    int frequencia;
    struct noArvore *direita;
    struct noArvore *esquerda;
} NoArvore;

typedef struct noLista {
    struct noArvore *no;
    struct noLista *proximo;
} NoLista;

// Lista e Arvore 
NoArvore *criarNoArvore(int frequencia, char caractere, NoArvore *direita, NoArvore *esquerda) {
    NoArvore *novo = (NoArvore*)malloc(sizeof(NoArvore));
    if (!novo) return NULL;
    novo->frequencia = frequencia;
    novo->caractere = caractere;
    novo->direita = direita;
    novo->esquerda = esquerda;
    return novo;
}

NoLista *criarNoLista(int frequencia, char caractere, NoArvore *direita, NoArvore *esquerda) {
    NoLista *novo = (NoLista*)malloc(sizeof(NoLista));
    if (!novo) return NULL;
    novo->no = criarNoArvore(frequencia, caractere, direita, esquerda);
    novo->proximo = NULL;
    return novo;
}

void inserirOrdenadoListaRL(NoLista **raiz, int frequencia, char caractere, NoArvore *direita, NoArvore *esquerda) {
    NoLista *novo = criarNoLista(frequencia, caractere, direita, esquerda);
    if (!*raiz || (*raiz)->no->frequencia > frequencia) {
        novo->proximo = *raiz;
        *raiz = novo;
        return;
    }

    NoLista *aux = *raiz;
    while (aux->proximo && aux->proximo->no->frequencia <= frequencia)
        aux = aux->proximo;

    novo->proximo = aux->proximo;
    aux->proximo = novo;
}

void inserirOrdenadoLista(NoLista **raiz, int frequencia, char caractere) {
    inserirOrdenadoListaRL(raiz, frequencia, caractere, NULL, NULL);
}

NoArvore *capturarNo(NoLista **raiz) {
    NoLista *aux = *raiz;
    *raiz = (*raiz)->proximo;
    NoArvore *no = aux->no;
    free(aux);
    return no;
}

NoArvore *montarArvoreDeHuffman(NoLista **raiz) {
    while ((*raiz)->proximo) {
        NoArvore *esquerda = capturarNo(raiz);
        NoArvore *direita = capturarNo(raiz);
        inserirOrdenadoListaRL(raiz, esquerda->frequencia + direita->frequencia, '*', direita, esquerda);
    }
    return (*raiz)->no;
}

int altura(NoArvore *raiz) {
    if (!raiz) return -1;
    int esq = altura(raiz->esquerda);
    int dir = altura(raiz->direita);
    return (esq > dir ? esq : dir) + 1;
}

void destruirArvore(NoArvore *raiz) {
    if (raiz) {
        destruirArvore(raiz->esquerda);
        destruirArvore(raiz->direita);
        free(raiz);
    }
}

void destruirLista(NoLista *raiz) {
    while (raiz) {
        NoLista *temp = raiz;
        raiz = raiz->proximo;
        free(temp);
    }
}

// Tabela de Frequencia e Dicionario
void contarFrequencia(int *tabela, const char *caminho) {
    FILE *entrada = fopen(caminho, "rb");
    if (!entrada) {
        printf("Erro ao abrir o arquivo para leitura.\n");
        exit(1);
    }

    int caractere;
    while ((caractere = fgetc(entrada)) != EOF) {
        tabela[(unsigned char)caractere]++;
    }
    fclose(entrada);
}

char **criarDicionario(int alturaMaxima) {
    char **dicionario = malloc(TAM * sizeof(char *));
    for (int i = 0; i < TAM; i++) {
        dicionario[i] = calloc(alturaMaxima + 1, sizeof(char));
    }
    return dicionario;
}

void montarDicionario(NoArvore *raiz, char **dicionario, char *memoria, int profundidade) {
    if (!raiz->direita && !raiz->esquerda) {
        memoria[profundidade] = '\0';
        strcpy(dicionario[(unsigned char)raiz->caractere], memoria);
        return;
    }

    if (raiz->esquerda) {
        memoria[profundidade] = '0';
        montarDicionario(raiz->esquerda, dicionario, memoria, profundidade + 1);
    }

    if (raiz->direita) {
        memoria[profundidade] = '1';
        montarDicionario(raiz->direita, dicionario, memoria, profundidade + 1);
    }
}

// Compactacao
void trocarExtensao(const char *nomeOriginal, const char *novaExtensao, char *nomeNovo) {
    strcpy(nomeNovo, nomeOriginal);
    char *ponto = strrchr(nomeNovo, '.');
    if (ponto) {
        *ponto = '\0';
    }
    strcat(nomeNovo, novaExtensao);
}

void escreverMetadados(FILE *saida, char **dicionario, uint64_t totalBits) {
    int numCaracteres = 0;
    for (int i = 0; i < TAM; i++) {
        if (dicionario[i][0] != '\0')
            numCaracteres++;
    }

    fwrite(&totalBits, sizeof(uint64_t), 1, saida);
    fwrite(&numCaracteres, sizeof(int), 1, saida);

    for (int i = 0; i < TAM; i++) {
        if (dicionario[i][0] != '\0') {
            unsigned char caractere = (unsigned char)i;
            unsigned char tamanho = (unsigned char)strlen(dicionario[i]);
            fwrite(&caractere, sizeof(unsigned char), 1, saida);
            fwrite(&tamanho, sizeof(unsigned char), 1, saida);

            unsigned char byte = 0;
            int bitCount = 0;
            for (int j = 0; j < tamanho; j++) {
                if (dicionario[i][j] == '1')
                    byte |= (1 << (7 - bitCount));
                bitCount++;
                if (bitCount == 8 || j == tamanho - 1) {
                    fwrite(&byte, sizeof(unsigned char), 1, saida);
                    byte = 0;
                    bitCount = 0;
                }
            }
        }
    }
}

void compactar(char **dicionario, const char *caminho) {
    FILE *entrada = fopen(caminho, "rb");
    if (!entrada) {
        printf("Erro ao abrir o arquivo.\n");
        exit(1);
    }

    char nomeSaida[512];
    trocarExtensao(caminho, ".huff", nomeSaida);
    FILE *saida = fopen(nomeSaida, "wb");
    if (!saida) {
        printf("Erro ao criar o arquivo de saída.\n");
        fclose(entrada);
        exit(1);
    }

    long long capacidade = 1024;
    char *buffer = malloc(capacidade);
    long long totalBits = 0;

    int caractere;
    while ((caractere = fgetc(entrada)) != EOF) {
        char *codigo = dicionario[(unsigned char)caractere];
        for (int i = 0; codigo[i]; i++) {
            if (totalBits + 1 >= capacidade) {
                capacidade *= 2;
                buffer = realloc(buffer, capacidade);
                if (!buffer) {
                    printf("Erro de memória.\n");
                    exit(1);
                }
            }
            buffer[totalBits++] = codigo[i];
        }
    }

    int bitsExtra = (8 - (totalBits % 8)) % 8;
    for (int i = 0; i < bitsExtra; i++) {
        buffer[totalBits++] = '0';
    }

    escreverMetadados(saida, dicionario, totalBits);

    for (long long i = 0; i < totalBits; i += 8) {
        unsigned char byte = 0;
        for (int j = 0; j < 8; j++) {
            if (buffer[i + j] == '1')
                byte |= (1 << (7 - j));
        }
        fwrite(&byte, sizeof(unsigned char), 1, saida);
    }

    free(buffer);
    fclose(entrada);
    fclose(saida);

    printf("\nArquivo compactado para '%s'\n", nomeSaida);
}

// Descompactacao
void descompactar(const char *nomeEntrada) {
    char nomeSaida[512];
    trocarExtensao(nomeEntrada, ".txt", nomeSaida);

    FILE *entrada = fopen(nomeEntrada, "rb");
    FILE *saida = fopen(nomeSaida, "wb");
    if (!entrada || !saida) {
        printf("Erro ao abrir arquivos.\n");
        exit(1);
    }

    uint64_t qtdBits;
    int qtdCaracteres;
    fread(&qtdBits, sizeof(uint64_t), 1, entrada);
    fread(&qtdCaracteres, sizeof(int), 1, entrada);

    char **dicionario = criarDicionario(256);
    for (int i = 0; i < qtdCaracteres; i++) {
        unsigned char caractere;
        unsigned char tamanho;
        fread(&caractere, sizeof(unsigned char), 1, entrada);
        fread(&tamanho, sizeof(unsigned char), 1, entrada);

        unsigned char byte = 0;
        int bitsLidos = 0, j = 0;
        while (j < tamanho) {
            if (bitsLidos == 0) fread(&byte, sizeof(unsigned char), 1, entrada);
            dicionario[caractere][j++] = (byte & (1 << (7 - bitsLidos))) ? '1' : '0';
            bitsLidos = (bitsLidos + 1) % 8;
        }
        dicionario[caractere][j] = '\0';
    }

    char *buffer = calloc(257, sizeof(char));
    int indiceBuffer = 0;
    uint64_t bitsLidos = 0;

    int byte;
    while (bitsLidos < qtdBits && (byte = fgetc(entrada)) != EOF) {
        for (int i = 7; i >= 0 && bitsLidos < qtdBits; i--) {
            buffer[indiceBuffer++] = (byte & (1 << i)) ? '1' : '0';
            buffer[indiceBuffer] = '\0';

            for (int caractere = 0; caractere < TAM; caractere++) {
                if (dicionario[caractere][0] != '\0' && strcmp(buffer, dicionario[caractere]) == 0) {
                    fputc(caractere, saida);
                    indiceBuffer = 0;
                    buffer[0] = '\0';
                    break;
                }
            }

            bitsLidos++;
        }
    }

    for (int i = 0; i < TAM; i++) free(dicionario[i]);
    free(dicionario);
    free(buffer);
    fclose(entrada);
    fclose(saida);

    printf("\nArquivo descompactado para '%s'\n", nomeSaida);
}

// Main e Menu
int menu() {
    int opcao;
    system(CLEAR);
    printf("Compactador de arquivos by Gabriel Soares\n\n");
    printf("[1] - Compactar arquivo\n");
    printf("[2] - Descompactar arquivo\n");
    printf("[0] - Sair\n");
    printf("\nEscolha: ");
    scanf("%d", &opcao);
    getchar();

    return opcao;
}

int main() {
    NoLista *raizLista = NULL;
    NoArvore *raizArvore = NULL;
    char caminho[512];
    int tabela[TAM] = {0};
    int opcao;

    do {
        switch (opcao = menu()) {
            case 0: {
                return 0;
            }   
            case 1: {
                printf("Digite o caminho absoluto do arquivo a ser compactado: ");
                fgets(caminho, sizeof(caminho), stdin);
                caminho[strcspn(caminho, "\n")] = '\0';

                memset(tabela, 0, sizeof(tabela));
                raizLista = NULL;
                raizArvore = NULL;

                contarFrequencia(tabela, caminho);

                for (int i = 0; i < TAM; i++)
                    if (tabela[i] != 0)
                        inserirOrdenadoLista(&raizLista, tabela[i], i);

                raizArvore = montarArvoreDeHuffman(&raizLista);
                int alturaArvore = altura(raizArvore);
                char memoria[alturaArvore + 1];

                char **dicionario = criarDicionario(alturaArvore);
                montarDicionario(raizArvore, dicionario, memoria, 0);

                compactar(dicionario, caminho);

                destruirArvore(raizArvore);
                for (int i = 0; i < TAM; i++) free(dicionario[i]);
                free(dicionario);
                break;
            }
            case 2: {
                printf("Digite o caminho absoluto do arquivo a ser descompactado (.huff): ");
                fgets(caminho, sizeof(caminho), stdin);
                caminho[strcspn(caminho, "\n")] = '\0';

                descompactar(caminho);
                break;
            }
            default: {
                printf("Opcao invalida.\n");
            }
        }
        printf("\nPressione qualquer tecla para continuar...\n");
        getchar();
    } while (opcao != 0);

    return 0;
}
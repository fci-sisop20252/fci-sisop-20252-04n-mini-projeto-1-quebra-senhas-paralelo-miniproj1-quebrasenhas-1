#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO TRABALHADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * * Este programa verifica um subconjunto do espaço de senhas, usando a biblioteca
 * MD5 FORNECIDA para calcular hashes e comparar com o hash alvo.
 * * Uso: ./worker <hash_alvo> <senha_inicial> <senha_final> <charset> <tamanho> <worker_id>
 * * EXECUTADO AUTOMATICAMENTE pelo coordinator através de fork() + execl()
 * SEU TRABALHO: Implementar os TODOs de busca e comunicação
 */

#define RESULT_FILE "password_found.txt"
#define PROGRESS_INTERVAL 100000  // Reportar progresso a cada N senhas

/**
 * Incrementa uma senha para a próxima na ordem lexicográfica (aaa -> aab -> aac...)
 * * @param password Senha atual (será modificada)
 * @param charset Conjunto de caracteres permitidos
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @return 1 se incrementou com sucesso, 0 se chegou ao limite (overflow)
 */
int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    
    // TODO 1: Implementar o algoritmo de incremento de senha
    // OBJETIVO: Incrementar senha como um contador (ex: aaa -> aab -> aac -> aad...)
    // DICA: Começar do último caractere, como somar 1 em um número
    // DICA: Se um caractere "estoura", volta ao primeiro e incrementa o caracter a esquerda (aay -> aaz -> aba)
    
    // IMPLEMENTE AQUI:
    // - Percorrer password de trás para frente
    // - Para cada posição, encontrar índice atual no charset
    // - Incrementar índice
    // - Se não estourou: atualizar caractere e retornar 1
    // - Se estourou: definir como primeiro caractere e continuar loop
    // - Se todos estouraram: retornar 0 (fim do espaço)
    
    int i;
    // Itera pela senha de trás pra frente
    for (i = password_len - 1; i >= 0; i--) {
        // Encontra o ponteiro do caracter atual
        char *char_ptr = strchr(charset, password[i]);
        // Calcula o índice
        int current_index = char_ptr - charset;

        // Incrementa o índice para obter o próximo caracter
        int next_index = current_index + 1;

        // Verifica se o novo índice está dentro dos limites do charset
        if (next_index < charset_len) {
            // Se não estourou, atualiza o caracter na senha.
            password[i] = charset[next_index];
            // Retorna 1 para indicar que o increment foi bem-sucedido
            return 1;
        } else {
            // Se estourou, reseta o caracter atual para o primeiro do charset
            password[i] = charset[0];
            // O loop continuará para a próxima posição à esquerda
        }
    }

    // Se o loop terminar, significa que todos os caracter estouraram (ex: 'zzz' -> 'aaa')
    // Isso indica o fim do espaço de senhas para este comprimento
    return 0;  // SUBSTITUA por sua implementação
}

/**
 * Compara duas senhas
 * * @return -1 se a < b, 0 se a == b, 1 se a > b
 */
int password_compare(const char *a, const char *b) {
    return strcmp(a, b);
}

/**
 * Verifica se o arquivo de resultado já existe
 * Usado para parada antecipada se outro worker já encontrou a senha
 */
int check_result_exists() {
    return access(RESULT_FILE, F_OK) == 0;
}

/**
 * Salva a senha encontrada no arquivo de resultado
 * Usa O_CREAT | O_EXCL para garantir escrita atômica (apenas um worker escreve)
 */
void save_result(int worker_id, const char *password) {
    // TODO 2: Implementar gravação atômica do resultado
    // OBJETIVO: Garantir que apenas UM worker escreva no arquivo
    // DICA: Use O_CREAT | O_EXCL - falha se arquivo já existe
    // FORMATO DO ARQUIVO: "worker_id:password\n"
    
    // IMPLEMENTE AQUI:
    // - Tentar abrir arquivo com O_CREAT | O_EXCL | O_WRONLY
    // - Se sucesso: escrever resultado e fechar
    // - Se falhou: outro worker já encontrou
    
    // Tenta abrir/criar o arquivo de resultado.
    // O_WRONLY: Modo de apenas escrita.
    // O_CREAT: Cria o arquivo se ele não existir.
    // O_EXCL: Falha se o arquivo já existir. A combinação O_CREAT | O_EXCL é atômica.
    // 0644: Permissões do arquivo (leitura/escrita para o dono, leitura para outros).
    int fd = open(RESULT_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
    
    // Verifica se a abertura do arquivo foi bem-sucedida
    if (fd >= 0) {
        // Aloca um buffer para a string de resultado
        char buffer[128];
        // Formata a string no formato worker_id:senha\n
        snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);
        // Escreve a string formatada no arquivo
        write(fd, buffer, strlen(buffer));
        // Fecha o descritor do arquivo para liberar o recurso
        close(fd);
    }
    // Se fd < 0, a abertura falhou, o que significa que outro worker já criou o arquivo
    // Nesse caso, o worker atual simplesmente ignora e continua seu processo de finalização
}

/**
 * Função principal do worker
 */
int main(int argc, char *argv[]) {
    // Validar argumentos
    if (argc != 7) {
        fprintf(stderr, "Uso interno: %s <hash> <start> <end> <charset> <len> <id>\n", argv[0]);
        return 1;
    }
    
    // Parse dos argumentos
    const char *target_hash = argv[1];
    char *start_password = argv[2];
    const char *end_password = argv[3];
    const char *charset = argv[4];
    int password_len = atoi(argv[5]);
    int worker_id = atoi(argv[6]);
    int charset_len = strlen(charset);
    
    printf("[Worker %d] Iniciado: %s até %s\n", worker_id, start_password, end_password);
    
    // Buffer para a senha atual
    char current_password[11];
    strcpy(current_password, start_password);
    
    // Buffer para o hash calculado
    char computed_hash[33];
    
    // Contadores para estatísticas
    long long passwords_checked = 0;
    time_t start_time = time(NULL);
    time_t last_progress_time = start_time;
    
    // Loop principal de verificação
    while (1) {
        // TODO 3: Verificar periodicamente se outro worker já encontrou a senha
        // DICA: A cada PROGRESS_INTERVAL senhas, verificar se arquivo resultado existe
        // Verifica se o número de senhas checadas é um múltiplo do intervalo definido.
        if (passwords_checked > 0 && passwords_checked % PROGRESS_INTERVAL == 0) {
            // Se for, verifica se o arquivo de resultado já existe.
            if (check_result_exists()) {
                // Se o arquivo existe, outro worker encontrou a senha
                printf("[Worker %d] Arquivo de resultado encontrado. Parando.\n", worker_id);
                // Interrompe o loop principal para encerrar o trabalho
                break;
            }
        }
        
        // TODO 4: Calcular o hash MD5 da senha atual
        // IMPORTANTE: Use a biblioteca MD5 FORNECIDA - md5_string(senha, hash_buffer)
        // Calcula o hash MD5 da senha atual e armazena o resultado em 'computed_hash'.
        md5_string(current_password, computed_hash);
        
        // TODO 5: Comparar com o hash alvo
        // Se encontrou: salvar resultado e terminar
        
        // TODO 6: Incrementar para a próxima senha
        // DICA: Use a função increment_password implementada acima
        
        // TODO: Verificar se chegou ao fim do intervalo
        // Se sim: terminar loop
        
        passwords_checked++;
    }
    
    // Estatísticas finais
    time_t end_time = time(NULL);
    double total_time = difftime(end_time, start_time);
    
    printf("[Worker %d] Finalizado. Total: %lld senhas em %.2f segundos", 
           worker_id, passwords_checked, total_time);
    if (total_time > 0) {
        printf(" (%.0f senhas/s)", passwords_checked / total_time);
    }
    printf("\n");
    
    return 0;
}
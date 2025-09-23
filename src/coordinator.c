#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
    // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1
    
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <hash> <tamanho> <charset> <workers>\n", argv[0]);
        fprintf(stderr, "Erro: argumentos insuficientes\n");
        return 1;
    }
    
    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);
    
    // TODO: Adicionar validações dos parâmetros
    // - password_len deve estar entre 1 e 10
    if (password_len < 1 || password_len > 10){
        printf("Tamanho de senha inválido!\n");
        return 1;
    }
    // - num_workers deve estar entre 1 e MAX_WORKERS
    if (num_workers <1 || num_workers >MAX_WORKERS){
        printf("Número de workers inválido!\n");
        return 1;
    }
    // - charset não pode ser vazio

    if (charset_len == 0){
        printf("Charset está vazio!");
        return 1;
    }
    
    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);
    
    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);
    
    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);
    
    // Registrar tempo de início
    time_t start_time = time(NULL);
    
    // TODO 2: Dividir o espaço de busca entre os workers
    // Calcular quantas senhas cada worker deve verificar
    // DICA: Use divisão inteira e distribua o resto entre os primeiros workers
    
    // IMPLEMENTE AQUI:
    long long passwords_per_worker = total_space/num_workers;  
    long long remaining = total_space % num_workers;
    
    // Arrays para armazenar PIDs dos workers
    pid_t array_workers[MAX_WORKERS];
    
    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando workers...\n");
    
    long long index_inicio = 0;
    // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {
        // TODO: Calcular intervalo de senhas para este worker
        long long count = passwords_per_worker;
        if (i < remaining) {
            count++;
        }
        long long index_fim = index_inicio + count -1;
        // TODO: Converter indices para senhas de inicio e fim
        char primeira_senha[password_len+1];
        char ultima_senha[password_len+1]; 
        index_to_password(index_inicio, charset, charset_len, password_len, primeira_senha);
        index_to_password(index_fim, charset, charset_len, password_len, ultima_senha);
        // TODO 4: Usar fork() para criar processo filho
        // TODO 6: No processo filho: usar execl() para executar worker
        // TODO 5: No processo pai: armazenar PID
        // TODO 7: Tratar erros de fork() e execl()
        pid_t pid = fork();
        if (pid < 0) {
            perror("Erro no fork!");
            exit(1);
            
        } 
        // APENAS O FILHO EXECUTA AQUI
        else if (pid == 0) {
            execl("./worker", "worker", target_hash, primeira_senha, ultima_senha, charset, (char *)NULL);
            perror("Erro em execl");
            exit(1);
        }         
        // APENAS O PAI EXECUTA AQUI
        else {
            printf("Worker %d criado (PID %d) -> intervalo [%s .. %s]\n", i, array_workers[i], primeira_senha, ultima_senha);  
        }
        index_inicio = index_fim + 1;
    }
         
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");
    
    // TODO 8: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    
    // IMPLEMENTE AQUI:
    // - Loop para aguardar cada worker terminar
    // - Usar wait() para capturar status de saída
    // - Identificar qual worker terminou
    // - Verificar se terminou normalmente ou com erro
    // - Contar quantos workers terminaram
    int workers_finalizados = 0;
    int status;

    while (workers_finalizados < num_workers) {
        pid_t pid = wait(&status); // espera qualquer filho
        if (pid == -1) {
            perror("Erro no wait()");
            exit(1);
        }

        if(WIFEXITED(status)){
            int exit_code = WEXITSTATUS(status);
                printf("Worker (PID %d) terminou normalmente com código %d\n", pid, exit_code);
        }
        else if (WIFSIGNALED(status)) {
                printf("Worker (PID %d) terminou por sinal %d\n", pid, WTERMSIG(status));
        }
        else {
                printf("Worker (PID %d) terminou de forma inesperada\n", pid);
        }

        workers_finalizados++;
    }
    
    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    
    printf("\n=== Resultado ===\n");
    
    // TODO 9: Verificar se algum worker encontrou a senha
    // Ler o arquivo password_found.txt se existir
    
    // IMPLEMENTE AQUI:
    // - Abrir arquivo RESULT_FILE para leitura
    // - Ler conteúdo do arquivo
    // - Fazer parse do formato "worker_id:password"
    // - Verificar o hash usando md5_string()
    // - Exibir resultado encontrado
    
    // Estatísticas finais (opcional)
    // TODO: Calcular e exibir estatísticas de performance
    FILE *fp = fopen(RESULT_FILE, "r");
    if (fp != NULL) {
        char linha[256];
        if (fgets(linha, sizeof(linha), fp) != NULL) {
            linha[strcspn(linha, "\n")] = 0;

            char *token = strtok(linha, ":");
            if (token != NULL) {
                int worker_id = atoi(token);
                char *senha = strtok(NULL, ":");

                if (senha != NULL) {
                    char hash_gerado[33];
                    md5_string(senha, hash_gerado);

                    if (strcmp(hash_gerado, target_hash) == 0) {
                        printf("Senha encontrada pelo worker %d: %s\n", worker_id, senha);
                    } else {
                        printf("Aviso: Senha lida do arquivo não corresponde ao hash!\n");
                    }
                }
            }
        }
        fclose(fp);
    } else {
        printf("Nenhum worker encontrou a senha.\n");
    }

    printf("Tempo total de execução: %.2f segundos\n", elapsed_time);
    
    return 0;
}
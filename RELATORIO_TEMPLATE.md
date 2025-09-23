# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** Nome (Matrícula), Nome (Matrícula),,,  
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

O espaço de busca é o total de combinações possíveis de senhas dado pelo tamanho do charset elevado ao comprimento da senha. Esse espaço foi dividido em intervalos contíguos para cada worker.
A estratégia foi: calcular a divisão inteira (total_space / num_workers) e distribuir o resto entre os primeiros workers, garantindo que todos percorram quase a mesma quantidade de combinações.
Cada worker recebe um índice inicial (index_inicio) e um índice final (index_fim), que são convertidos em senhas para definir o intervalo de busca.

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
long long passwords_per_worker = total_space / num_workers;  
long long remaining = total_space % num_workers;

long long index_inicio = 0;
for (int i = 0; i < num_workers; i++) {
    long long count = passwords_per_worker;
    if (i < remaining) {
        count++;
    }
    long long index_fim = index_inicio + count - 1;

    char primeira_senha[password_len+1];
    char ultima_senha[password_len+1]; 
    index_to_password(index_inicio, charset, charset_len, password_len, primeira_senha);
    index_to_password(index_fim, charset, charset_len, password_len, ultima_senha);

    // criação do worker via fork/exec (mostrado abaixo)
    index_inicio = index_fim + 1;
}

```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

No coordinator, cada worker é criado com a chamada fork(). O processo filho (worker) usa execl() para executar o programa worker, recebendo como argumentos o hash alvo, senha inicial, senha final, charset e outros parâmetros. Já o processo pai guarda o PID do filho e continua criando os demais.
Após a criação, o coordinator entra em um loop com wait() para aguardar todos os filhos terminarem.

**Código do fork/exec:**
```c
pid_t pid = fork();
if (pid < 0) {
    perror("Erro no fork!");
    exit(1);
} 
else if (pid == 0) {
    execl("./worker", "worker", target_hash, primeira_senha, ultima_senha, charset,
          argv[2], argv[4], (char *)NULL);
    perror("Erro em execl");
    exit(1);
}         
else {
    printf("Worker %d criado (PID %d) -> intervalo [%s .. %s]\n",
           i, pid, primeira_senha, ultima_senha);  
}


Loop do wait (coordinator.c):

int workers_finalizados = 0;
int status;

while (workers_finalizados < num_workers) {
    pid_t pid = wait(&status);
    if (pid == -1) {
        perror("Erro no wait()");
        exit(1);
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        printf("Worker (PID %d) terminou normalmente com código %d\n", pid, exit_code);
    } else if (WIFSIGNALED(status)) {
        printf("Worker (PID %d) terminou por sinal %d\n", pid, WTERMSIG(status));
    } else {
        printf("Worker (PID %d) terminou de forma inesperada\n", pid);
    }
    workers_finalizados++;
}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**

Foi usada a chamada de sistema open() com as flags O_CREAT | O_EXCL | O_WRONLY.
Isso significa que o arquivo só é criado se ele não existir. Se já existir, a chamada falha, impedindo que outro worker sobrescreva o resultado. Essa técnica garante escrita atômica e evita condições de corrida, pois apenas o primeiro worker que encontra a senha consegue salvar.
Leia sobre condições de corrida (aqui)[https://pt.stackoverflow.com/questions/159342/o-que-%C3%A9-uma-condi%C3%A7%C3%A3o-de-corrida]

**Como o coordinator consegue ler o resultado?**

Após todos os workers terminarem, o coordinator abre o arquivo password_found.txt em modo leitura. Ele lê uma linha no formato "worker_id:senha", separa o worker_id e a senha, recalcula o hash MD5 da senha e compara com o hash alvo.
Se for igual, imprime a senha encontrada e qual worker a descobriu. Caso o arquivo não exista, significa que nenhum worker encontrou a senha no intervalo dado.

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
|  ___s | ________s| ________s | ________s | ___________s |

**O speedup foi linear? Por quê?**
[Analise se dobrar workers realmente dobrou a velocidade e explique o overhead de criar processos]

---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
[Descreva um problema e como resolveu. Ex: "Tive dificuldade com o incremento de senha, mas resolvi tratando-o como um contador em base variável"]

---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [ ] Código compila sem erros
- [ ] Todos os TODOs foram implementados
- [ ] Testes passam no `./tests/simple_test.sh`
- [ ] Relatório preenchido
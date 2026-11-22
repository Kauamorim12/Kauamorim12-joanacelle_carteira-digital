#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===== Constantes ===== */

#define TAM_NOME        50
#define MAX_CONTAS      5
#define MAX_HISTORICO   50

#define NUMERO_INICIAL  1000
#define NUMERO_MAXIMO   9999   /* número da conta tem 4 dígitos */

/* Códigos do tipo de conta */
#define CORRENTE        1
#define SALARIO         2
#define POUPANCA        3

/* Limite do empréstimo = 25% de 10 vezes o saldo total */
#define FATOR_SALDO         10
#define PERCENTUAL_LIMITE   0.25

/* ===== Structs ===== */

/* Uma linha do histórico */
typedef struct {
    char  operacao[30];
    float valor;
    float saldoApos;
} Movimento;

typedef struct {
    int   numero;
    int   tipo;
    float saldo;
    Movimento historico[MAX_HISTORICO];
    int   qtdHistorico;
} Conta;

typedef struct {
    char  nome[TAM_NOME];
    Conta contas[MAX_CONTAS];
    int   qtdContas;
    int   temEmprestimo;      /* só pode um por cliente */
} Cliente;

/* ===== Variáveis globais ===== */

Cliente *clientes = NULL;            /* vetor dinâmico de clientes */
int qtdClientes = 0;                 /* clientes cadastrados */
int capacidade = 0;                  /* clientes que cabem na memória reservada */
int bancoAberto = 0;
int proximoNumero = NUMERO_INICIAL;

/* ===== Leitura do teclado ===== */

/* Descarta o resto da linha (o '\n' que o scanf deixa). */
void limparBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

/* Lê um inteiro; repete até a entrada ser válida. */
int lerInt(char msg[]) {
    int valor, ok;
    do {
        printf("%s", msg);
        ok = scanf("%d", &valor);   /* devolve quantos valores leu */
        limparBuffer();
        if (ok != 1) {
            printf("Valor inválido. Digite um número inteiro.\n");
        }
    } while (ok != 1);
    return valor;
}

/* Lê um número real (com ponto: 10.50); repete até ser válido. */
float lerFloat(char msg[]) {
    float valor;
    int ok;
    do {
        printf("%s", msg);
        ok = scanf("%f", &valor);
        limparBuffer();
        if (ok != 1) {
            printf("Valor inválido. Digite um número (use ponto, ex.: 10.50).\n");
        }
    } while (ok != 1);
    return valor;
}

/* Lê um texto com espaços e remove o '\n' do final. */
void lerTexto(char msg[], char destino[], int tam) {
    int n;
    printf("%s", msg);
    fgets(destino, tam, stdin);
    n = strlen(destino);
    if (n > 0 && destino[n - 1] == '\n') {
        destino[n - 1] = '\0';
    } else {
        limparBuffer();             /* texto maior que o vetor: descarta o excesso */
    }
}

/* ===== Conta ===== */

void mostrarTipo(int tipo) {
    switch (tipo) {
        case CORRENTE: printf("Corrente"); break;
        case SALARIO:  printf("Salário");  break;
        case POUPANCA: printf("Poupança"); break;
    }
}

/* Formato: 1000-01 | Corrente | Saldo: R$ 50.00 (número + código do tipo) */
void mostrarConta(Conta *c) {
    printf("%d-%02d | ", c->numero, c->tipo);
    mostrarTipo(c->tipo);
    printf(" | Saldo: R$ %.2f\n", c->saldo);
}

/* Grava no histórico; se estiver cheio, descarta a movimentação mais antiga. */
void registrar(Conta *c, char operacao[], float valor) {
    int i;

    if (c->qtdHistorico == MAX_HISTORICO) {
        for (i = 1; i < MAX_HISTORICO; i++) {
            c->historico[i - 1] = c->historico[i];   /* puxa cada linha uma posição */
        }
        c->qtdHistorico--;
    }

    strcpy(c->historico[c->qtdHistorico].operacao, operacao);
    c->historico[c->qtdHistorico].valor = valor;
    c->historico[c->qtdHistorico].saldoApos = c->saldo;
    c->qtdHistorico++;
}

/* Devolve 1 se depositou, 0 se o valor for inválido. */
int depositar(Conta *c, float valor, char operacao[]) {
    if (valor <= 0) {
        printf("Erro: o valor deve ser maior que zero.\n");
        return 0;
    }
    c->saldo += valor;
    registrar(c, operacao, valor);
    return 1;
}

/* Devolve 1 se sacou, 0 se o valor for inválido ou o saldo insuficiente. */
int sacar(Conta *c, float valor) {
    if (valor <= 0) {
        printf("Erro: o valor deve ser maior que zero.\n");
        return 0;
    }
    if (valor > c->saldo) {
        printf("Erro: saldo insuficiente.\n");
        return 0;
    }
    c->saldo -= valor;
    registrar(c, "Saque", valor);
    return 1;
}

/* ===== Cliente ===== */

/* Devolve 1 se o cliente tem alguma conta do tipo informado. */
int temConta(Cliente *cl, int tipo) {
    int i;
    for (i = 0; i < cl->qtdContas; i++) {
        if (cl->contas[i].tipo == tipo) {
            return 1;
        }
    }
    return 0;
}

float saldoTotal(Cliente *cl) {
    float total = 0;
    int i;
    for (i = 0; i < cl->qtdContas; i++) {
        total += cl->contas[i].saldo;
    }
    return total;
}

float limiteEmprestimo(Cliente *cl) {
    return PERCENTUAL_LIMITE * FATOR_SALDO * saldoTotal(cl);
}

/* Valida as regras e cria a conta. Devolve 1 se conseguiu, 0 se não. */
int abrirConta(Cliente *cl, int tipo, float saldoInicial) {
    Conta *c;

    if (tipo < CORRENTE || tipo > POUPANCA) {
        printf("Erro: tipo inválido (use 1, 2 ou 3).\n");
        return 0;
    }
    if (saldoInicial < 0) {
        printf("Erro: o depósito inicial não pode ser negativo.\n");
        return 0;
    }
    if (cl->qtdContas >= MAX_CONTAS) {
        printf("Erro: o cliente atingiu o limite de %d contas.\n", MAX_CONTAS);
        return 0;
    }
    if (proximoNumero > NUMERO_MAXIMO) {
        printf("Erro: acabaram os números de conta.\n");
        return 0;
    }

    /* Não pode ter conta-salário e conta corrente ao mesmo tempo */
    if ((tipo == SALARIO && temConta(cl, CORRENTE)) ||
        (tipo == CORRENTE && temConta(cl, SALARIO))) {
        printf("Erro: o cliente não pode ter conta-salário e conta corrente ao mesmo tempo.\n");
        return 0;
    }

    c = &cl->contas[cl->qtdContas];   /* próxima posição livre */
    c->numero = proximoNumero;
    c->tipo = tipo;
    c->saldo = 0;
    c->qtdHistorico = 0;
    proximoNumero++;

    registrar(c, "Abertura de conta", 0);
    if (saldoInicial > 0) {
        depositar(c, saldoInicial, "Depósito inicial");
    }

    cl->qtdContas++;
    printf("Conta criada: ");
    mostrarConta(c);
    return 1;
}

/* ===== Banco ===== */

/* Lista os clientes e devolve a posição escolhida no vetor (ou -1). */
int escolherCliente() {
    int i, op;

    if (qtdClientes == 0) {
        printf("Nenhum cliente cadastrado.\n");
        return -1;
    }
    for (i = 0; i < qtdClientes; i++) {
        printf("%d - %s\n", i + 1, clientes[i].nome);
    }
    op = lerInt("Escolha o cliente (número): ");
    if (op < 1 || op > qtdClientes) {
        printf("Cliente inválido.\n");
        return -1;
    }
    return op - 1;    /* a lista começa em 1, o vetor em 0 */
}

/* Lista as contas do cliente e devolve a posição escolhida (ou -1). */
int escolherConta(Cliente *cl) {
    int i, op;

    if (cl->qtdContas == 0) {
        printf("Este cliente ainda não tem contas.\n");
        return -1;
    }
    for (i = 0; i < cl->qtdContas; i++) {
        printf("%d - ", i + 1);
        mostrarConta(&cl->contas[i]);
    }
    op = lerInt("Escolha a conta (número da lista): ");
    if (op < 1 || op > cl->qtdContas) {
        printf("Conta inválida.\n");
        return -1;
    }
    return op - 1;
}

/* Cadastra um cliente; quando o vetor enche, dobra a capacidade com realloc. */
void cadastrarCliente() {
    char nome[TAM_NOME];
    Cliente *novo;
    int novaCapacidade;

    lerTexto("Nome do novo cliente: ", nome, TAM_NOME);
    if (strlen(nome) == 0) {
        printf("Erro: o nome não pode ser vazio.\n");
        return;
    }

    if (qtdClientes == capacidade) {
        if (capacidade == 0) {
            novaCapacidade = 2;
        } else {
            novaCapacidade = capacidade * 2;
        }
        novo = realloc(clientes, novaCapacidade * sizeof(Cliente));
        if (novo == NULL) {               /* falhou: o vetor antigo continua válido */
            printf("Erro: falta de memória.\n");
            return;
        }
        clientes = novo;
        capacidade = novaCapacidade;
    }

    strcpy(clientes[qtdClientes].nome, nome);
    clientes[qtdClientes].qtdContas = 0;
    clientes[qtdClientes].temEmprestimo = 0;
    qtdClientes++;
    printf("Cliente cadastrado!\n");
}

void opAbrirConta() {
    int pos, tipo;
    float inicial;

    pos = escolherCliente();
    if (pos == -1) {
        return;
    }
    printf("Tipos: 1 - Corrente | 2 - Salário | 3 - Poupança\n");
    tipo = lerInt("Tipo da conta: ");
    inicial = lerFloat("Depósito inicial (0 para nenhum): R$ ");
    abrirConta(&clientes[pos], tipo, inicial);
}

void opDepositar() {
    int pos, posConta;
    float valor;

    pos = escolherCliente();
    if (pos == -1) {
        return;
    }
    posConta = escolherConta(&clientes[pos]);
    if (posConta == -1) {
        return;
    }
    valor = lerFloat("Valor do depósito: R$ ");
    if (depositar(&clientes[pos].contas[posConta], valor, "Depósito")) {
        printf("Depósito realizado! Novo saldo: R$ %.2f\n", clientes[pos].contas[posConta].saldo);
    }
}

void opSacar() {
    int pos, posConta;
    float valor;

    pos = escolherCliente();
    if (pos == -1) {
        return;
    }
    posConta = escolherConta(&clientes[pos]);
    if (posConta == -1) {
        return;
    }
    valor = lerFloat("Valor do saque: R$ ");
    if (sacar(&clientes[pos].contas[posConta], valor)) {
        printf("Saque realizado! Novo saldo: R$ %.2f\n", clientes[pos].contas[posConta].saldo);
    }
}

/* Exige: nenhum empréstimo anterior, conta corrente e poupança com saldo.
 * O valor é creditado na conta corrente. */
void opEmprestimo() {
    int pos, i, temPoupancaComSaldo = 0;
    int posCorrente = -1;
    float valor, limite;
    Cliente *cl;

    pos = escolherCliente();
    if (pos == -1) {
        return;
    }
    cl = &clientes[pos];

    if (cl->temEmprestimo) {
        printf("Não é possível: o cliente já possui um empréstimo (máximo de 1).\n");
        return;
    }

    for (i = 0; i < cl->qtdContas; i++) {
        if (cl->contas[i].tipo == CORRENTE && posCorrente == -1) {
            posCorrente = i;
        }
        if (cl->contas[i].tipo == POUPANCA && cl->contas[i].saldo > 0) {
            temPoupancaComSaldo = 1;
        }
    }
    if (posCorrente == -1) {
        printf("Não é possível: o cliente precisa ter uma conta corrente.\n");
        return;
    }
    if (!temPoupancaComSaldo) {
        printf("Não é possível: o cliente precisa ter saldo na conta-poupança.\n");
        return;
    }

    limite = limiteEmprestimo(cl);   /* calculado antes de creditar o empréstimo */
    printf("Limite disponível: R$ %.2f\n", limite);
    valor = lerFloat("Valor do empréstimo: R$ ");

    if (valor <= 0) {
        printf("Erro: o valor deve ser maior que zero.\n");
    } else if (valor > limite) {
        printf("Erro: valor acima do limite.\n");
    } else {
        depositar(&cl->contas[posCorrente], valor, "Crédito de empréstimo");
        cl->temEmprestimo = 1;
        printf("Empréstimo concedido! Valor creditado na conta corrente.\n");
    }
}

void opListarClientes() {
    int i, j;

    if (qtdClientes == 0) {
        printf("Nenhum cliente cadastrado.\n");
        return;
    }
    for (i = 0; i < qtdClientes; i++) {
        printf("\n%s | Saldo total: R$ %.2f\n", clientes[i].nome, saldoTotal(&clientes[i]));
        for (j = 0; j < clientes[i].qtdContas; j++) {
            printf("   ");
            mostrarConta(&clientes[i].contas[j]);
        }
    }
}

/* Submenu de operações; o banco continua aberto ao voltar. */
void menuBancoAberto() {
    int opcao;

    bancoAberto = 1;
    printf("\n*** BANCO ABERTO ***\n");

    do {
        printf("\n--- Operações ---\n");
        printf("1 - Cadastrar cliente\n");
        printf("2 - Abrir conta\n");
        printf("3 - Depositar\n");
        printf("4 - Sacar\n");
        printf("5 - Solicitar empréstimo\n");
        printf("6 - Listar clientes\n");
        printf("0 - Voltar ao menu principal (o banco continua aberto)\n");
        opcao = lerInt("Escolha: ");

        switch (opcao) {
            case 1: cadastrarCliente(); break;
            case 2: opAbrirConta();     break;
            case 3: opDepositar();      break;
            case 4: opSacar();          break;
            case 5: opEmprestimo();     break;
            case 6: opListarClientes(); break;
            case 0: break;
            default: printf("Opção inválida.\n");
        }
    } while (opcao != 0);
}

/* Mostra o histórico de todas as contas do cliente escolhido. */
void consultarHistorico() {
    int pos, i, j;
    Cliente *cl;

    pos = escolherCliente();
    if (pos == -1) {
        return;
    }
    cl = &clientes[pos];

    if (cl->qtdContas == 0) {
        printf("Este cliente ainda não tem contas.\n");
        return;
    }
    for (i = 0; i < cl->qtdContas; i++) {
        printf("\nHistórico da conta %d-%02d\n", cl->contas[i].numero, cl->contas[i].tipo);
        for (j = 0; j < cl->contas[i].qtdHistorico; j++) {
            printf("   %s: R$ %.2f | Saldo: R$ %.2f\n",
                   cl->contas[i].historico[j].operacao,
                   cl->contas[i].historico[j].valor,
                   cl->contas[i].historico[j].saldoApos);
        }
    }
}

void efetuarSorteio() {
    int sorteado;

    if (qtdClientes == 0) {
        printf("Não há clientes cadastrados para o sorteio.\n");
        return;
    }
    sorteado = rand() % qtdClientes;    /* posição de 0 a qtdClientes-1 */
    printf("Cliente sorteado: %s\n", clientes[sorteado].nome);
}

/* ===== Programa principal ===== */

int main() {
    int opcao;

#ifdef _WIN32
    system("chcp 65001 > nul");   /* UTF-8 no terminal do Windows (acentos) */
#endif

    srand(time(NULL));   /* semente diferente a cada execução */

    do {
        printf("\n===== CARTEIRA DIGITAL =====\n");
        printf("1 - Abrir o banco\n");
        printf("2 - Consultar histórico\n");
        printf("3 - Efetuar sorteio\n");
        printf("0 - Sair\n");
        opcao = lerInt("Escolha: ");

        switch (opcao) {
            case 1:
                menuBancoAberto();
                break;
            case 2:                       /* exige banco aberto */
                if (bancoAberto) {
                    consultarHistorico();
                } else {
                    printf("O banco está fechado. Abra o banco primeiro (opção 1).\n");
                }
                break;
            case 3:                       /* exige banco aberto */
                if (bancoAberto) {
                    efetuarSorteio();
                } else {
                    printf("O banco está fechado. Abra o banco primeiro (opção 1).\n");
                }
                break;
            case 0:
                printf("Encerrando. Até logo!\n");
                break;
            default:
                printf("Opção inválida.\n");
        }
    } while (opcao != 0);

    if (clientes != NULL) {
        free(clientes);
    }
    return 0;
}

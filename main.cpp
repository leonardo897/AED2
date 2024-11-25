#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int max_pacientes = 1000;
int max_salas = 5;
int max_medicos = 5;
int max_especialidades = 5;
#define DIAS 6 // Segunda à Sábado
#define HORARIO_INICIO 9 // 9:00
#define HORARIO_FIM 18 // 18:00
#define CHANCE_FALTA 5
#define ARQUIVO_SAIDA "saida.txt" //Arquivo de saida
#define ARQUIVO_ENTRADA "entrada.dat" //Arquivo de entrada


typedef struct {
    int id;
    char nome[50];
} Especialidade;

// Estrutura para armazenar os dados de um médico
typedef struct {
    int id;
    char nome[50];
    char especialidade[50]; // Cada médico tem uma especialidade fixa
    int horas_trabalhadas;
    bool atendendo;
} Medico;

// Estrutura para armazenar os dados de um paciente
typedef struct {
    int id;
    char nome[50];
    int idade;
    char sintomas[50];
    float prioridade;
    char especialidade[50];
    int faltas; // 0 = não faltou, 1 = faltou uma vez, 2 = faltou duas vezes
    int retorno; // dias utéis até o retorno
} Paciente;

// Estrutura para armazenar os dados de uma sala
typedef struct {
    int num;
} Sala;

//Heap
typedef struct {
    Paciente *pacientes;
    int tamanho;
    int capacidade;
} Heap;

// Função para criar um novo heap
Heap* criar_heap(int capacidade) {
    Heap *heap = (Heap*)malloc(sizeof(Heap));
    heap->pacientes = (Paciente*)malloc(sizeof(Paciente) * capacidade);
    heap->tamanho = 0;
    heap->capacidade = capacidade;
    return heap;
}

// Função para trocar dois pacientes de lugar no heap
void trocar(Paciente *p1, Paciente *p2) {
    Paciente temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

// Função para manter a propriedade do heap (max-heap) a partir de um índice
void arrumar_heap(Heap *heap, int indice) {
    int maior = indice;
    int esquerda = 2 * indice + 1;
    int direita = 2 * indice + 2;

    if (esquerda < heap->tamanho && heap->pacientes[esquerda].prioridade > heap->pacientes[maior].prioridade) {
        maior = esquerda;
    }

    if (direita < heap->tamanho && heap->pacientes[direita].prioridade > heap->pacientes[maior].prioridade) {
        maior = direita;
    }

    if (maior != indice) {
        trocar(&heap->pacientes[indice], &heap->pacientes[maior]);
        arrumar_heap(heap, maior);
    }
}

// Função para inserir um paciente no heap
void inserir_paciente(Heap *heap, Paciente paciente) {
    if (heap->tamanho == heap->capacidade) {
        printf("Capacidade do heap atingida!\n");
        return;
    }

    heap->pacientes[heap->tamanho] = paciente;
    int i = heap->tamanho;
    heap->tamanho++;

    while (i > 0 && heap->pacientes[(i - 1) / 2].prioridade < heap->pacientes[i].prioridade) {
        trocar(&heap->pacientes[i], &heap->pacientes[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

// Comparar as horas trabalhadas dos médicos
int compararHorasTrabalhadas(const void *a, const void *b) {
    Medico *medicoA = (Medico *)a;
    Medico *medicoB = (Medico *)b;

    // Ordena em ordem decrescente com base nas horas trabalhadas
    return medicoB->horas_trabalhadas - medicoA->horas_trabalhadas;
}

// Função para remover o paciente com maior prioridade (raiz do heap)
Paciente remover_paciente(Heap *heap) {
    if (heap->tamanho == 0) {
        printf("Heap vazio!\n");
        Paciente vazio = {0};
        return vazio;
    }

    Paciente paciente = heap->pacientes[0];
    heap->pacientes[0] = heap->pacientes[heap->tamanho - 1];
    heap->tamanho--;
    arrumar_heap(heap, 0);

    return paciente;
}

// Os pacientes da fila de espera são adicionados de volta a fila comum
void resetar_fila(Heap *fila, Heap *fila_espera){
    while(fila_espera->tamanho > 0){
        Paciente paciente = remover_paciente(fila_espera);
        inserir_paciente(fila, paciente);
    }
}

// Marcar as consultas para os pacientes
void marcar_consultas(Heap *fila, Heap *fila_espera, Medico medicos[], Sala salas[]){
    int semana = 1;
    int dia = 1;
    int hora = HORARIO_INICIO;
    int num_sala = 0;

    FILE *saida = fopen(ARQUIVO_SAIDA, "w");//Abrir arquivo de saida

    fprintf(saida, "SEMANA %d:\n", semana);
    fprintf(saida, "DIA %d:\n", dia);
    fprintf(saida, "HORA: %d:00\n", hora);

    while(fila->tamanho > 0){
        Paciente paciente_atual = remover_paciente(fila); //Chama o proximo paciente

        if(rand() % 100 < CHANCE_FALTA){ //Verifica se o paciente vai faltar (5% de chance)
            paciente_atual.faltas++;

            if(paciente_atual.faltas == 1){ // Se o paciente tiver uma falta, ele vai para a fila de espera com prioridade reduzida
                paciente_atual.prioridade /= 2;

                inserir_paciente(fila_espera, paciente_atual);
            }
            if(fila->tamanho == 0){
                resetar_fila(fila, fila_espera);
            }
            continue;
        }

        if(paciente_atual.retorno > 1){// Se não é o dia do retorno o paciente volta para a fila de espera
            inserir_paciente(fila_espera, paciente_atual);
            if(fila->tamanho == 0){
                resetar_fila(fila, fila_espera);
            }
            paciente_atual = remover_paciente(fila);
        }

        Sala *sala;
        Medico *medico;

        //Procurar o médico com a especialidade correta
        for(int i = 0; i < max_medicos; i++){

            if (strcmp(paciente_atual.especialidade, medicos[i].especialidade) == 0){
                medico = &medicos[i];

                break;
            }
        }

        //Verifica se o médico está disponível
        if(medico->atendendo){
            inserir_paciente(fila_espera, paciente_atual);
            // Se o medico está atendendo o paciente vai para a fila de espera
        }

        else{
            //A consulta é agendada na sala
            sala = &salas[num_sala];
            num_sala++;

            fprintf(saida, "SALA %d - MÉDICO: %s - PACIENTE: %s\n", sala->num, medico->nome, paciente_atual.nome);

            medico->atendendo = true;
            medico->horas_trabalhadas++;// Aumentar as horas trabalhadas do médico

            if(paciente_atual.retorno == 0){// Verifica se é a primeira consulta
                paciente_atual.retorno = rand() % 30 + 2; // Agenda a consulta de retorno para até 30 dias utéis
                // O paciente vai para a fila de espera com prioridade aumentada
                paciente_atual.prioridade *= 2;
                inserir_paciente(fila_espera, paciente_atual);
            }
        }

        if(num_sala == max_salas || fila->tamanho == 0){
            //Atualizar horario e data
            hora++;
            if (hora == HORARIO_FIM){
                hora = HORARIO_INICIO;
                dia++;
                // Conta o dia util para a consulta de retorno
                for (int i = 0; i < fila->tamanho; i++){
                    if(fila->pacientes[i].retorno > 0){
                        fila->pacientes[i].retorno--;
                    }
                }

                for (int i = 0; i < fila_espera->tamanho; i++){
                    if(fila_espera->pacientes[i].retorno > 0){
                        fila_espera->pacientes[i].retorno--;
                    }
                }

                if(dia == DIAS + 1){
                    dia = 1;
                    semana++;
                    fprintf(saida, "SEMANA %d:\n", semana);
                    fprintf(saida, "DIA %d:\n", dia);
                }
                else{
                    fprintf(saida, "DIA %d:\n", dia);
                }
            }
            fprintf(saida, "HORA: %d:00\n", hora);

            // Medicos acabam o atendimento
            for(int i = 0; i < max_medicos; i++){
                medicos[i].atendendo = false;
            }
            num_sala = 0;
            resetar_fila(fila, fila_espera);
        }
    }

    int total = 0;
    fprintf(saida, "\nHORAS TRABALHAS POR CADA MÉDICO:\n");
    qsort(medicos, max_medicos, sizeof(Medico), compararHorasTrabalhadas);
    for(int i = 0; i < max_medicos; i++){
        fprintf(saida, "%s: %d horas\n", medicos[i].nome, medicos[i].horas_trabalhadas);
        total += medicos[i].horas_trabalhadas;
    }
    fprintf(saida, "\nTOTAL: %d horas\n", total);

    // Fechar arquivos
    fclose(saida);
}

int main() {

    srand(time(NULL));

    FILE *entrada = fopen(ARQUIVO_ENTRADA, "rb");//Abrir arquivo de entrada

    // Ler os dados do arquivo de entrada

    fread(&max_pacientes, sizeof(int), 1, entrada);

    Heap *fila = criar_heap(max_pacientes);
    Heap *fila_espera = criar_heap(max_pacientes);

    Paciente paciente;
    for (int i = 0; i < max_pacientes; i++) {
        fread(&paciente, sizeof(Paciente), 1, entrada);
        paciente.prioridade = (rand() % 100 + 1);
        inserir_paciente(fila, paciente);
    }

    fread(&max_salas, sizeof(int), 1, entrada);

    Sala salas[max_salas];

    fread(salas, sizeof(Sala), max_salas, entrada);

    fread(&max_especialidades, sizeof(int), 1, entrada);

    Especialidade especialidades[max_especialidades];

    fread(especialidades, sizeof(Especialidade), max_especialidades, entrada);

    fread(&max_medicos, sizeof(int), 1, entrada);

    Medico medicos[max_medicos];

    fread(medicos, sizeof(medicos), max_medicos, entrada);

    fclose(entrada); // Fechar arquivo de entrada

    marcar_consultas(fila, fila_espera, medicos, salas);

    //Liberar espaço alocado
    free(fila->pacientes);
    free(fila);

    free(fila_espera->pacientes);
    free(fila_espera);

    return 0;
}

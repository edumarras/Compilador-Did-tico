#define _WIN32_IE 0x0300

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <unistd.h>

#define GUI_MODE
#define ID_ARQUIVO_ABRIR 1
#define ID_SOBRE 2
#define ID_EDITBOX 3
#define ID_LISTBOX 6
#define ID_BOTAO_COMPILAR 7
#define ID_TIMER 5
#define MAX_LEXEMA 100

const char g_szClassName[] = "CompiladorClass";

// Definição do tipo de token
typedef enum {
TOKEN_DIGITO,
TOKEN_IDENTIFICADOR,
TOKEN_PALAVRA_RESERVADA,
TOKEN_ATRIBUICAO,
TOKEN_OPERADOR_ARITMETICO,
TOKEN_OPERADOR_RELACIONAL,
TOKEN_PONTUACAO,
TOKEN_ERRO,
TOKEN_SPROGRAMA,
TOKEN_SSE,
TOKEN_SENTAO,
TOKEN_SSENAO,
TOKEN_SENQUANTO,
TOKEN_SFACA,
TOKEN_SINICIO,
TOKEN_SFIM,
TOKEN_SESCREVA,
TOKEN_SLEIA,
TOKEN_SVAR,
TOKEN_SINTEIRO,
TOKEN_SBOOLEANO,
TOKEN_SVERDADEIRO,
TOKEN_SFALSO,
TOKEN_SPROCEDIMENTO,
TOKEN_SFUNCAO,
TOKEN_SDIV,
TOKEN_SE,
TOKEN_SOU,
TOKEN_SNAO,
TOKEN_SPONTO_VIRGULA,
TOKEN_SVIRGULA,
TOKEN_SABRE_PARENTESES,
TOKEN_SFECHA_PARENTESES,
TOKEN_SPONTO,
TOKEN_SMAIS,
TOKEN_SMENOS,
TOKEN_SMULT,
TOKEN_SERRO,
TOKEN_SATRIB,
TOKEN_SDOISPONTOS,
TOKEN_SMAIOR,
TOKEN_SMENOR,
TOKEN_SIGUAL,
TOKEN_SDIFERENTE,
TOKEN_SMAIOR_IGUAL,
TOKEN_SMENOR_IGUAL
// Outros tipos de tokens
} TipoToken;

typedef struct {
    TipoToken simbolo;
    char lexema[100]; // Tamanho arbitrário, ajuste conforme necessário
} Token;

// Definição da estrutura de um nó da tabela de símbolos
typedef struct Node {
    char lexema[MAX_LEXEMA];
    int escopo;
    char tipo[20];
    int memoria; // vai ser o rótulo
    int marcacao; // talvez possa tirar
    struct Node* next;
} Node;

typedef struct {
    Token tokens[100];
    int quantidade;
    char tipo[20];      // útil para verificação de tipos
    bool em_analise;    // útil durante a coleta
} ListaTokens;

//Variáveis Globais

// Da GUI
HWND hEdit;
HWND hStatus;
HWND hListBox;
HWND hBtnCompilar;

// Do Compilador
int linhas;
Token token;
FILE *arquivo;
int caractere;
Node* tabelaSimbolos;
int nivel;
ListaTokens lista_expressao;
ListaTokens exp_posfix;

// Protótipos da GUI
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CompiladorGUI(HWND hwnd);
void CriarMenu(HWND hwnd);
void CriarAreaTexto(HWND hwnd);
void CriarAreaErros(HWND hwnd);
void CriarBarraStatus(HWND hwnd);
void AtualizarPosicaoCursor(void);
void MenuAbrir(HWND hwnd);

// Protótipos do seu Compilador
Token TrataDigito(int *caractere, FILE *arquivo);
Token TrataIdentificadorPalavraReservada(int *caractere, FILE *arquivo);
Token TrataAtribuicao(int *caractere, FILE *arquivo);
Token TrataOperadorAritmetico(int *caractere, FILE *arquivo);
Token TrataOperadorRelacional(int *caractere, FILE *arquivo);
Token TrataPontuacao(int *caractere, FILE *arquivo);
void RetornaLexema(Token *token_als, char string[]);
char* verifica_tipo_operacao(Token operador, char* tipo1, char* tipo2);
Node* createNode(const char* lexema, int escopo, const char* tipo, int memoria);
void inserirNaTabela(Node** top, const char* lexema, int escopo, char* tipo, int memoria);
Node* pesquisa_declvar_tabela(Node* top, const char* lexema);
Node* pesquisa_declproc_tabela(Node* top, const char* lexema);
Node* pesquisa_declvarfunc_tabela(Node* top, const char* lexema);
Node* pesquisa_tabela(Node* top, const char* lexema, int nivel);
bool Pesquisa_duplicvar_tabela(Node* top, const char* lexema);
Node* pesquisa_declfunc_tabela(Node*top, const char* lexema);
void TABSIMB(Node* top, const char* lexema, int i);
void colocarTipoNasVariaveis(Node** top, const char* novoTipo);
void imprimirTabela(Node* top);
void Desempilha(Node** top, int nivel);
int LerCaracter(FILE *arquivo);
Token PegaToken(int *caractere, FILE *arquivo);
void adiciona_token_expressao(Token t);
void limpa_lista_expressao(void);
bool eh_operador(TipoToken tipo);
bool eh_operador_unario(Token* tokens, int pos);
int precedencia(Token token, bool eh_unario);
void imprime_lista_tokens(ListaTokens *lista, char* nome);
void converte_para_posfix(void);
void Analisa_chamada_funcao(void);
void Analisa_chamada_de_procedimento(void);
void Analisa_fator(void);
void Analisa_termo(void);
void Analisa_expressao_simples(void);
void Analisa_expressao(void);
void Analisa_declaracao_funcao(void);
void Analisa_declaracao_procedimento(void);
void Analisa_se(void);
void Analisa_enquanto(void);
void Analisa_escreva(void);
void Analisa_leia(void);
void Analisa_atribuicao(void);
void Analisa_atrib_chprocedimento(void);
void Analisa_comando_simples(void);
void Analisa_comandos(void);
void Analisa_tipo(void);
void Analisa_subrotinas(void);
void Analisa_variaveis(void);
void Analisa_et_variaveis(void);
void Analisa_bloco(void);
void AnalisadorSintatico(void);
void erro(void);
Token LerProximoToken(FILE *arquivo, int *caractere);
void contador_linhas(FILE *arquivo, int caractere, int linhas);

// Adicione essa função junto com suas outras funções
void CompiladorGUI(HWND hwnd) {
    // Primeiro, limpa a lista de erros anterior
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    // Pega o tamanho do texto na área de edição
    int tamanho = GetWindowTextLength(hEdit);
    if(tamanho <= 0) {
        MessageBox(hwnd, "Editor está vazio!", "Aviso", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Aloca memória para o texto
    char* texto = (char*)malloc(tamanho + 1);
    if(texto == NULL) {
        MessageBox(hwnd, "Erro de memória!", "Erro", MB_OK | MB_ICONERROR);
        return;
    }

    // Pega o texto da área de edição
    GetWindowText(hEdit, texto, tamanho + 1);

    // Cria um arquivo temporário
    FILE* temp = fopen("temp_compile.txt", "w");
    if(temp == NULL) {
        MessageBox(hwnd, "Erro ao criar arquivo temporário!", "Erro", MB_OK | MB_ICONERROR);
        free(texto);
        return;
    }

    // Escreve o texto no arquivo
    fputs(texto, temp);
    fclose(temp);
    free(texto);

    // Aqui vamos chamar seu compilador usando o arquivo temporário
    // ... por enquanto só vamos mostrar que salvou
    MessageBox(hwnd, "Arquivo temporário criado com sucesso!", "Sucesso", MB_OK | MB_ICONINFORMATION);
}

// Função para criar menus
void CriarMenu(HWND hwnd) {
    HMENU hMenu = CreateMenu();
    HMENU hArquivoMenu = CreatePopupMenu();

    // Menu Arquivo
    AppendMenu(hArquivoMenu, MF_STRING, ID_ARQUIVO_ABRIR, "Abrir");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hArquivoMenu, "Arquivo");

    // Menu Sobre
    AppendMenu(hMenu, MF_STRING, ID_SOBRE, "Sobre");

    SetMenu(hwnd, hMenu);
}

void CriarAreaTexto(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    hEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",    // Voltamos para EDIT normal
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        10,
        10,
        rect.right - 20,
        rect.bottom - 40,
        hwnd,
        (HMENU)ID_EDITBOX,
        GetModuleHandle(NULL),
        NULL
    );

    HFONT hFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas"
    );

    SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void CriarAreaErros(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Criar a listbox
    hListBox = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "LISTBOX",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        10,
        rect.bottom - 150,  // 150 pixels do fundo
        rect.right - 100,   // Deixa espaço para o botão
        110,                // Altura da listbox
        hwnd,
        (HMENU)ID_LISTBOX,
        GetModuleHandle(NULL),
        NULL
    );

    // Criar o botão Compilar
    hBtnCompilar = CreateWindowEx(
        0,
        "BUTTON",
        "Compilar",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rect.right - 80,    // 80 pixels da direita
        rect.bottom - 150,  // Alinha com a listbox
        70,                 // Largura do botão
        30,                 // Altura do botão
        hwnd,
        (HMENU)ID_BOTAO_COMPILAR,
        GetModuleHandle(NULL),
        NULL
    );

    // Usar a mesma fonte que já usamos antes
    HFONT hFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas"
    );

    SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCompilar, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void CriarBarraStatus(HWND hwnd) {
    // Criar a barra de status
    hStatus = CreateWindowEx(
        0,                          // sem estilo extendido
        STATUSCLASSNAME,            // classe da barra de status
        "",                         // sem texto inicial
        WS_CHILD | WS_VISIBLE,      // estilos da janela
        0, 0, 0, 0,                 // x, y, largura, altura (será ajustado automaticamente)
        hwnd,                       // janela pai
        (HMENU)ID_STATUSBAR,        // ID do controle
        GetModuleHandle(NULL),      // instância
        NULL                        // sem dados extras
    );

    // Definir a fonte
    HFONT hFont = CreateFont(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );

    SendMessage(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// Função para atualizar a posição do cursor
void AtualizarPosicaoCursor(void) {
    if (!hEdit || !hStatus) return;  // Proteção contra ponteiros nulos

    // Pegar a seleção atual
    DWORD inicio, fim;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&inicio, (LPARAM)&fim);

    // Pegar a linha atual
    int linha = SendMessage(hEdit, EM_LINEFROMCHAR, inicio, 0) + 1;

    // Pegar o índice do início da linha
    int inicioLinha = SendMessage(hEdit, EM_LINEINDEX, linha - 1, 0);

    // Calcular a coluna
    int coluna = inicio - inicioLinha + 1;

    // Atualizar a barra de status
    char texto[50];
    sprintf(texto, "Linha: %d, Coluna: %d", linha, coluna);
    SetWindowText(hStatus, texto);
}

void MenuAbrir(HWND hwnd) {
    OPENFILENAME ofn;
    char nomeArquivo[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Arquivos de Texto (*.txt)\0*.txt\0Todos os Arquivos (*.*)\0*.*\0";
    ofn.lpstrFile = nomeArquivo;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "txt";

    if(GetOpenFileName(&ofn)) {
        FILE *arquivo = fopen(nomeArquivo, "rt"); // 'rt' para modo texto
        if(arquivo == NULL) {
            MessageBox(hwnd, "Erro ao abrir o arquivo!", "Erro", MB_OK | MB_ICONERROR);
            return;
        }

        // Primeiro, vamos até o fim do arquivo para saber seu tamanho
        fseek(arquivo, 0, SEEK_END);
        long tamanho = ftell(arquivo);
        fseek(arquivo, 0, SEEK_SET);  // Volta para o início

        // Aloca memória para o conteúdo (+1 para o terminador nulo)
        char* conteudo = (char*)malloc(tamanho + 1);
        if(conteudo == NULL) {
            MessageBox(hwnd, "Erro de memória!", "Erro", MB_OK | MB_ICONERROR);
            fclose(arquivo);
            return;
        }

        // Lê o arquivo inteiro de uma vez
        size_t bytesLidos = fread(conteudo, 1, tamanho, arquivo);
        conteudo[bytesLidos] = '\0';  // Adiciona terminador nulo

        fclose(arquivo);

        // Converte todos os \n para \r\n se necessário
        char* conteudoFinal = (char*)malloc(tamanho * 2 + 1); // Dobro do tamanho para caso precise converter
        if(conteudoFinal == NULL) {
            free(conteudo);
            MessageBox(hwnd, "Erro de memória!", "Erro", MB_OK | MB_ICONERROR);
            return;
        }

        char* src = conteudo;
        char* dst = conteudoFinal;
        while(*src) {
            if(*src == '\n' && (src == conteudo || *(src-1) != '\r')) {
                *dst++ = '\r';
                *dst++ = '\n';
            } else {
                *dst++ = *src;
            }
            src++;
        }
        *dst = '\0';

        // Coloca o conteúdo na área de texto
        SetWindowText(hEdit, conteudoFinal);

        // Libera a memória
        free(conteudo);
        free(conteudoFinal);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
            CriarMenu(hwnd);
            CriarAreaTexto(hwnd);
            CriarAreaErros(hwnd);  // Adicionado
            CriarBarraStatus(hwnd);
            SetTimer(hwnd, ID_TIMER, 100, NULL);
            break;

        case WM_TIMER:
            if(wParam == ID_TIMER) {
                AtualizarPosicaoCursor();
            }
            break;

        case WM_SIZE:
            {
                RECT rect;
                GetClientRect(hwnd, &rect);

                // Redimensionar a área de texto (agora menor para acomodar a lista)
                MoveWindow(hEdit, 10, 10,
                          rect.right - 20,
                          rect.bottom - 180,  // Deixa espaço para lista e status
                          TRUE);

                // Redimensionar a lista de erros
                MoveWindow(hListBox, 10, rect.bottom - 150,
                          rect.right - 100, 110, TRUE);

                // Reposicionar o botão compilar
                MoveWindow(hBtnCompilar, rect.right - 80, rect.bottom - 150,
                          70, 30, TRUE);

                // Redimensionar a barra de status
                MoveWindow(hStatus, 0, rect.bottom - 20,
                          rect.right, 20, TRUE);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case ID_ARQUIVO_ABRIR:
                    MenuAbrir(hwnd);
                    break;

                case ID_SOBRE:
                    MessageBox(hwnd,
                        "Compilador desenvolvido para disciplina de Compiladores\n\n"
                        "Este compilador realiza análise léxica, sintática e semântica.",
                        "Sobre o Compilador",
                        MB_OK | MB_ICONINFORMATION);
                    break;

                case ID_EDITBOX:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        AtualizarPosicaoCursor();
                    }
                    break;

                case ID_BOTAO_COMPILAR:
    CompiladorGUI(hwnd);
    break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
   LPSTR lpCmdLine, int nCmdShow)
{

   INITCOMMONCONTROLSEX icc;
   icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icc.dwICC = ICC_WIN95_CLASSES;
   InitCommonControlsEx(&icc);

   WNDCLASSEX wc;
   HWND hwnd;
   MSG Msg;

   // Registrando a classe da janela
   wc.cbSize        = sizeof(WNDCLASSEX);
   wc.style         = 0;
   wc.lpfnWndProc   = WndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
   wc.lpszMenuName  = NULL;
   wc.lpszClassName = g_szClassName;
   wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

   if(!RegisterClassEx(&wc))
   {
       MessageBox(NULL, "Falha no Registro da Janela!", "Erro!",
           MB_ICONEXCLAMATION | MB_OK);
       return 0;
   }

   // Criando a janela
   hwnd = CreateWindowEx(
       WS_EX_CLIENTEDGE,
       g_szClassName,
       "Compilador",
       WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,   // x, y, width, height
       NULL, NULL, hInstance, NULL);

   if(hwnd == NULL)
   {
       MessageBox(NULL, "Falha na Criação da Janela!", "Erro!",
           MB_ICONEXCLAMATION | MB_OK);
       return 0;
   }

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   // Loop de mensagens
   while(GetMessage(&Msg, NULL, 0, 0) > 0)
   {
       TranslateMessage(&Msg);
       DispatchMessage(&Msg);
   }
   return Msg.wParam;
}

//Compilador

// criar novo nó
Node* createNode(const char* lexema, int escopo, const char* tipo, int memoria) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        printf("Erro ao alocar memória!\n");
        exit(1);
    }
    strcpy(newNode->lexema, lexema);
    newNode->escopo = escopo;
    strcpy(newNode->tipo, tipo);
    newNode->memoria = memoria;
    newNode->marcacao = 0;
    newNode->next = NULL;
    return newNode;
}

// insere na tabela
void inserirNaTabela(Node** top, const char* lexema, int escopo, char* tipo, int memoria){
    Node* newNode = createNode(lexema, escopo, tipo, memoria);
    newNode->next = *top;
    *top = newNode;
    printf("Inserido: %s\n", lexema);
}

// Função para consultar a tabela
Node* pesquisa_declvar_tabela(Node* top, const char* lexema) {
    Node* current = top;
    while (current != NULL) {
        if (strcmp(current->lexema, lexema) == 0) {
                if(strcmp(current->tipo, "inteiro") == 0){ //só pode ler inteiro
                    return current;
                }
                else{
                    return NULL;
                }
        }
        current = current->next;
    }
    return NULL;
}

Node* pesquisa_declproc_tabela(Node* top, const char* lexema) {
    Node* current = top;
    while (current != NULL) {
        if (strcmp(current->lexema, lexema) == 0) { // mudar aqui caso importe o escopo
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Node* pesquisa_declvarfunc_tabela(Node* top, const char* lexema) {
   Node* current = top;
   while (current != NULL) {
       if (strcmp(current->lexema, lexema) == 0) {
           return current;
       }
       current = current->next;
   }
   return NULL;
}

Node* pesquisa_tabela(Node* top, const char* lexema, int nivel) {
    Node* current = top;
    Node* global_match = NULL;  // para guardar match do escopo global

    printf("Analisador Léxico encontrou: Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);

    while (current != NULL) {
        if (strcmp(current->lexema, lexema) == 0) {
            if (current->escopo == nivel) {
                printf("\n\nVariavel encontrada no escopo %d\n\n", nivel);
                return current;  // Retorna imediatamente se encontrar no escopo atual
            }
            else if (current->escopo == 0 && global_match == NULL) {
                global_match = current;  // Guarda match global, mas continua procurando
            }
        }
        current = current->next;
    }

    if (global_match != NULL) {
        printf("\n\nVariavel encontrada no escopo global\n\n");
        return global_match;
    }

    printf("\n\nVariavel %s nao encontrada nos escopos %d e global\n\n", lexema, nivel);
    return NULL;
}

bool Pesquisa_duplicvar_tabela(Node* top, const char* lexema) {
    Node* current = top;
    while (current != NULL) {
        if (current->escopo == 1) {  // Se encontrar escopo = 1, para a busca
            return false;
        }
        if (strcmp(current->lexema, lexema) == 0) {  // Se encontrar lexema igual, é duplicata
            return true;
        }
        current = current->next;
    }
    return false;
}

Node* pesquisa_declfunc_tabela(Node*top, const char* lexema){
    Node* current = top;
    while(current != NULL){
        if(strcmp(current->lexema, lexema) == 0){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void TABSIMB(Node* top, const char* lexema, int i) { // auxiliar é o nome na tabela
    Node* current = top;
    while (current != NULL) {
        if (strcmp(current->lexema, lexema) == 0 && strcmp(current->tipo, "funcao") == 0) {
            if (i == 0) {
                strcpy(current->tipo, "funcao inteiro");
            } else {
                strcpy(current->tipo, "funcao booleana");
            }
        }
        current = current->next;
    }
}

// Função para colocar tipo nas variáveis
void colocarTipoNasVariaveis(Node** top, const char* novoTipo) {
   Node* current = *top;

   while (current != NULL) {
       if (strcmp(current->tipo, "variavel") == 0) {
           strcpy(current->tipo, novoTipo);
       }
       current = current->next;
   }
}

// Função para imprimir a tabela
void imprimirTabela(Node* top) {
    Node* current = top;
    printf("\nTabela de Símbolos:\n");
    printf("Lexema\t\tEscopo\t\tTipo\t\tMemória\n");
    while (current != NULL) {
        printf("%s\t\t%d\t\t%s\t\t%d\n", current->lexema, current->escopo, current->tipo, current->memoria);
        current = current->next;
    }
    printf("\n");
}

// Função para remover elementos do escopo atual
void Desempilha(Node** top, int nivel) {
    Node* current = *top;
    Node* proc_node = NULL;
    bool encontrou_marca = false;

    // Primeiro encontra o nó do procedimento (que está marcado)
    while (current != NULL && !encontrou_marca) {
        if (current->escopo == 1) { // Encontrou o procedimento marcado
            encontrou_marca = true;
            proc_node = current;
            proc_node->escopo = 0; // Remove a marca do procedimento
            break;
        }
        current = current->next;
    }

    // Se encontrou o procedimento marcado, remove os nós acima dele
    if (encontrou_marca) {
        current = *top;
        Node* temp;

        // Remove todos os nós que estão acima do procedimento
        while (current != proc_node) {
            temp = current;
            current = current->next;
            *top = current;
            free(temp);
        }
    }
}

// Função para ler o próximo caractere do arquivo
int LerCaracter(FILE *arquivo) {
    int c = fgetc(arquivo);
    if (c == '\n') {
        linhas++;
    }
    return c;
}

// Função principal para pegar o próximo token
Token PegaToken(int *caractere, FILE *arquivo) {
    while (isspace(*caractere)) { //ignora espaço e quebra de linha
        *caractere = LerCaracter(arquivo);
    }

    if (isdigit(*caractere)) {
        return TrataDigito(caractere, arquivo);
    } else if (isalpha(*caractere) || *caractere == '_') {
        return TrataIdentificadorPalavraReservada(caractere, arquivo);
    } else if (*caractere == ':') {
        return TrataAtribuicao(caractere, arquivo);
    } else if (*caractere == '+' || *caractere == '-' || *caractere == '*') {
        return TrataOperadorAritmetico(caractere, arquivo);
    } else if (*caractere == '!' || *caractere == '<' || *caractere == '>' || *caractere == '=') {
        return TrataOperadorRelacional(caractere, arquivo);
    } else if (*caractere == ';' || *caractere == ',' || *caractere == '(' || *caractere == ')' || *caractere == '.') {
        return TrataPontuacao(caractere, arquivo);
    } else {
        Token token;
        token.simbolo = TOKEN_ERRO;
        token.lexema[0] = '\0';
        *caractere = LerCaracter(arquivo);
        return token;
    }
}

// Implementação de TrataDigito
Token TrataDigito(int *caractere, FILE *arquivo) {
    Token token;
    token.simbolo = TOKEN_DIGITO;
    char num[100]; // Tamanho arbitrário, ajuste conforme necessário
    int i = 0;

    // Inicia a construção do número
    num[i++] = *caractere;
    *caractere = LerCaracter(arquivo);

    // Continua lendo enquanto for dígito
    while (isdigit(*caractere)) {
        num[i++] = *caractere;
        *caractere = LerCaracter(arquivo);
    }

    num[i] = '\0'; // Finaliza a string com o número

    // Armazena no token
    strcpy(token.lexema, num);

    return token;
}

// Implementação de TrataIdentificadorPalavraReservada
Token TrataIdentificadorPalavraReservada(int *caractere, FILE *arquivo) { //quando duas palavras reservadas estao juntas ela é considerada identificador e nao duas palavras separadas, isso se torna um erro no sintatico
    Token token;
    char id[100]; // Tamanho arbitrário, ajuste conforme necessário
    int i = 0;

    // Inicia a construção do identificador
    id[i++] = *caractere;
    *caractere = LerCaracter(arquivo);

    // Continua lendo enquanto for letra, dígito ou "_"
    while (isalnum(*caractere) || *caractere == '_') {
        id[i++] = *caractere;
        *caractere = LerCaracter(arquivo);
    }

    id[i] = '\0'; // Finaliza a string com o identificador
    // Verifica se é uma palavra reservada
    if (strcmp(id, "programa") == 0) {
         RetornaLexema(&token, "programa");
        token.simbolo = TOKEN_SPROGRAMA;
    } else if (strcmp(id, "se") == 0) {
         RetornaLexema(&token, "se");
        token.simbolo = TOKEN_SSE;
    } else if (strcmp(id, "entao") == 0) {
        RetornaLexema(&token, "entao");
        token.simbolo = TOKEN_SENTAO;
    } else if (strcmp(id, "senao") == 0) {
        RetornaLexema(&token, "senao");
        token.simbolo = TOKEN_SSENAO;
    } else if (strcmp(id, "enquanto") == 0) {
        RetornaLexema(&token, "enquanto");
        token.simbolo = TOKEN_SENQUANTO;
    } else if (strcmp(id, "faca") == 0) {
        RetornaLexema(&token, "faca");
        token.simbolo = TOKEN_SFACA;
    } else if (strcmp(id, "inicio") == 0) {
        RetornaLexema(&token, "inicio");
        token.simbolo = TOKEN_SINICIO;
    } else if (strcmp(id, "fim") == 0) {
        RetornaLexema(&token, "fim");
        token.simbolo = TOKEN_SFIM;
    } else if (strcmp(id, "escreva") == 0) {
        RetornaLexema(&token, "escreva");
        token.simbolo = TOKEN_SESCREVA;
    } else if (strcmp(id, "leia") == 0) {
        RetornaLexema(&token, "leia");
        token.simbolo = TOKEN_SLEIA;
    } else if (strcmp(id, "var") == 0) {
        RetornaLexema(&token, "var");
        token.simbolo = TOKEN_SVAR;
    } else if (strcmp(id, "inteiro") == 0) {
        RetornaLexema(&token, "inteiro");
        token.simbolo = TOKEN_SINTEIRO;
    } else if (strcmp(id, "booleano") == 0) {
        RetornaLexema(&token, "booleano");
        token.simbolo = TOKEN_SBOOLEANO;
    } else if (strcmp(id, "verdadeiro") == 0) {
        RetornaLexema(&token, "verdadeiro");
        token.simbolo = TOKEN_SVERDADEIRO;
    } else if (strcmp(id, "falso") == 0) {
        RetornaLexema(&token, "falso");
        token.simbolo = TOKEN_SFALSO;
    } else if (strcmp(id, "procedimento") == 0) {
        RetornaLexema(&token, "procedimento");
        token.simbolo = TOKEN_SPROCEDIMENTO;
    } else if (strcmp(id, "funcao") == 0) {
        RetornaLexema(&token, "funcao");
        token.simbolo = TOKEN_SFUNCAO;
    } else if (strcmp(id, "div") == 0) {
        RetornaLexema(&token, "div");
        token.simbolo = TOKEN_SDIV;
    } else if (strcmp(id, "e") == 0) {
        RetornaLexema(&token, "e");
        token.simbolo = TOKEN_SE;
    } else if (strcmp(id, "ou") == 0) {
        RetornaLexema(&token, "ou");
        token.simbolo = TOKEN_SOU;
    } else if (strcmp(id, "nao") == 0) {
        RetornaLexema(&token, "nao");
        token.simbolo = TOKEN_SNAO;
    } else {
        // Se não for palavra reservada, é um identificador
        RetornaLexema(&token, id);
        token.simbolo = TOKEN_IDENTIFICADOR;
    }

    return token;
}

// Implementação de TrataAtribuicao
Token TrataAtribuicao(int *caractere, FILE *arquivo) {
    Token token;
    *caractere = LerCaracter(arquivo);

    if (*caractere == '=') {
        *caractere = LerCaracter(arquivo);
        token.simbolo = TOKEN_ATRIBUICAO;
        RetornaLexema(&token,":=");
    } else {
        token.simbolo = TOKEN_SDOISPONTOS;
        RetornaLexema(&token,":");
    }

    return token;
}

// Implementação de TrataOperadorAritmetico
Token TrataOperadorAritmetico(int *caractere, FILE *arquivo) {
    Token token;
    switch (*caractere) {
        case '+':
            token.simbolo = TOKEN_SMAIS;
            RetornaLexema(&token,"+");
            break;
        case '-':
            token.simbolo = TOKEN_SMENOS;
            RetornaLexema(&token,"-");
            break;
        case '*':
            token.simbolo = TOKEN_SMULT;
            RetornaLexema(&token,"*");
            break;
        default:
            token.simbolo = TOKEN_ERRO;
            break;
    }
    *caractere = LerCaracter(arquivo);
    return token;
}

// Implementação de TrataOperadorRelacional
Token TrataOperadorRelacional(int *caractere, FILE *arquivo) {
    Token token;
    // Verifica o tipo de operador relacional e atribui o símbolo correspondente
    switch (*caractere) {

        case '!':
            *caractere = LerCaracter(arquivo);
        if (*caractere == '=') {
            token.simbolo = TOKEN_SDIFERENTE;
            RetornaLexema(&token,"!=");
           *caractere = LerCaracter(arquivo);

        } else {
            token.simbolo = TOKEN_ERRO;
            RetornaLexema(&token,"!");
        }
        break;

            case '<':
            *caractere = LerCaracter(arquivo);
            if (*caractere == '=') {
            token.simbolo = TOKEN_SMENOR_IGUAL;
            RetornaLexema(&token,"<=");
            *caractere = LerCaracter(arquivo);

        } else {
            token.simbolo = TOKEN_SMENOR;
            RetornaLexema(&token,"<");
            }
            break;

        case '>':
            *caractere = LerCaracter(arquivo);
            if (*caractere == '=') {
            token.simbolo = TOKEN_SMAIOR_IGUAL;
            RetornaLexema(&token,">=");
            *caractere = LerCaracter(arquivo);

        } else {
            token.simbolo = TOKEN_SMAIOR;
            RetornaLexema(&token,">");
            }
            break;

        case '=':
            *caractere = LerCaracter(arquivo);
            token.simbolo = TOKEN_SIGUAL;
            RetornaLexema(&token,"=");
            break;

        default:
            token.simbolo = TOKEN_ERRO;
            break;
    }
        return token;
}

// Implementação de TrataPontuacao
Token TrataPontuacao(int *caractere, FILE *arquivo) { // lexema cagado.
    Token token;
    switch (*caractere) {
        case ';':
            token.simbolo = TOKEN_SPONTO_VIRGULA;
            RetornaLexema(&token,";");
            break;
        case ',':
            token.simbolo = TOKEN_SVIRGULA;
            RetornaLexema(&token,",");
            break;
        case '(':
            token.simbolo = TOKEN_SABRE_PARENTESES;
            RetornaLexema(&token,"(");
            break;
        case ')':
            token.simbolo = TOKEN_SFECHA_PARENTESES;
            RetornaLexema(&token,")");
            break;
        case '.':
            token.simbolo = TOKEN_SPONTO;
            RetornaLexema(&token,".");
            break;
        default:
            token.simbolo = TOKEN_ERRO;
            break;
    }
    *caractere = LerCaracter(arquivo);
    return token;
}

void RetornaLexema(Token *token_als, char string[]) {
    strcpy((*token_als).lexema, string);
}

Token LerProximoToken(FILE *arquivo, int *caractere) {
    Token token;

    // Ignora espaços em branco e quebras de linha
    while (isspace(*caractere)) {
        *caractere = LerCaracter(arquivo);
    }

    if (*caractere == EOF) {
        token.simbolo = TOKEN_ERRO;
        strcpy(token.lexema, "EOF");
        return token;
    }

    // Trata comentários
    if (*caractere == '{') {
        while (*caractere != '}' && *caractere != EOF) {
            *caractere = LerCaracter(arquivo);
        }
        if (*caractere == '}') {
            *caractere = LerCaracter(arquivo);
        }
        return LerProximoToken(arquivo, caractere);  // Recursão para pegar o próximo token após o comentário
    }

    return PegaToken(caractere, arquivo);
}

void adiciona_token_expressao(Token t) {
    // Se não está em análise, limpa completamente a lista antes de começar
    if (!lista_expressao.em_analise) {
        printf("\nIniciando nova expressão\n");
        lista_expressao.quantidade = 0;
        lista_expressao.em_analise = true;
        strcpy(lista_expressao.tipo, "");
        // Zera todos os tokens para garantir
        memset(lista_expressao.tokens, 0, sizeof(Token) * 100);
    }

    // Proteção contra overflow
    if (lista_expressao.quantidade >= 100) {
        printf("\nErro: Expressão muito longa!\n");
        erro();
        return;
    }

    lista_expressao.tokens[lista_expressao.quantidade++] = t;
    printf("\nToken adicionado à expressão atual: '%s' (%s)",
           t.lexema, Nomedotoken[t.simbolo]);
}

void limpa_lista_expressao() {
    lista_expressao.quantidade = 0;
    lista_expressao.em_analise = false;
    strcpy(lista_expressao.tipo, "");
    memset(lista_expressao.tokens, 0, sizeof(Token) * 100);
    printf("\nLista de expressão limpa\n");
}

// Verifica se um token é operador
bool eh_operador(TipoToken tipo) {
    return (tipo == TOKEN_SMAIS || tipo == TOKEN_SMENOS ||
            tipo == TOKEN_SMULT || tipo == TOKEN_SDIV ||
            tipo == TOKEN_SMAIOR || tipo == TOKEN_SMENOR ||
            tipo == TOKEN_SIGUAL || tipo == TOKEN_SDIFERENTE ||
            tipo == TOKEN_SMAIOR_IGUAL || tipo == TOKEN_SMENOR_IGUAL ||
            tipo == TOKEN_SE || tipo == TOKEN_SOU);
}

// Verifica se é operador unário (+ ou - no início ou após outro operador/abre parênteses)
bool eh_operador_unario(Token* tokens, int pos) {
    if (pos == 0) return true;  // Primeiro token
    TipoToken anterior = tokens[pos-1].simbolo;
    return (eh_operador(anterior) || anterior == TOKEN_SABRE_PARENTESES);
}

// Define precedência dos operadores
int precedencia(Token token, bool eh_unario) {
    if (eh_unario) return 4;  // Operador unário tem precedência máxima

    switch(token.simbolo) {
        case TOKEN_SMULT:
        case TOKEN_SDIV:
            return 3;
        case TOKEN_SMAIS:
        case TOKEN_SMENOS:
            return 2;
        case TOKEN_SMAIOR:
        case TOKEN_SMENOR:
        case TOKEN_SIGUAL:
        case TOKEN_SDIFERENTE:
        case TOKEN_SMAIOR_IGUAL:
        case TOKEN_SMENOR_IGUAL:
            return 1;
        case TOKEN_SE:
        case TOKEN_SOU:
            return 0;
        default:
            return -1;
    }
}

// Função para debug - pode ser útil durante o desenvolvimento
void imprime_lista_tokens(ListaTokens *lista, char* nome) {
    printf("\n-----------------------------");
    printf("\nDebug: %s", nome);
    printf("\nQuantidade de tokens: %d", lista->quantidade);
    printf("\nTipo resultante: %s", lista->tipo);
    printf("\nEm análise: %s", lista->em_analise ? "sim" : "não");
    printf("\n-----------------------------");

    printf("\nTokens:");
    for(int i = 0; i < lista->quantidade; i++) {
        printf("\n%d: ", i);
        printf("Lexema: '%-15s' ", lista->tokens[i].lexema);
        printf("Token: %-30s", Nomedotoken[lista->tokens[i].simbolo]);
    }
    printf("\n-----------------------------\n\n");
}

void converte_para_posfix() {
    printf("\nIniciando conversão para posfix:");
    imprime_lista_tokens(&lista_expressao, "Lista de Entrada");
    exp_posfix.quantidade = 0;
    strcpy(exp_posfix.tipo, "");
    exp_posfix.em_analise = false;

    Token pilha[100];
    int topo_pilha = 0;

    // Arrays auxiliares para guardar os tipos durante a avaliação
    char tipos_operandos[100][20];  // guarda tipo de cada operando
    int num_tipos = 0;

    if (lista_expressao.quantidade == 0) {
        printf("\nErro: Não há expressão para converter!\n");
        return;
    }

    for(int i = 0; i < lista_expressao.quantidade; i++) {
        Token atual = lista_expressao.tokens[i];

        // Tratamento de operandos com verificação de tipo
        if(atual.simbolo == TOKEN_DIGITO) {
            exp_posfix.tokens[exp_posfix.quantidade++] = atual;
            strcpy(tipos_operandos[num_tipos++], "inteiro");
            continue;
        }

        if(atual.simbolo == TOKEN_IDENTIFICADOR) {
            Node* var = pesquisa_tabela(tabelaSimbolos, atual.lexema, nivel);
            if(var == NULL) {
                printf("\nErro: Variável '%s' não declarada!\n", atual.lexema);
                erro();
                return;
            }
            exp_posfix.tokens[exp_posfix.quantidade++] = atual;

            if(strcmp(var->tipo, "procedimento") == 0) {
                printf("\nErro: '%s' é um procedimento, não uma variável!\n", atual.lexema);
                erro();
                return;
            }
            strcpy(tipos_operandos[num_tipos++], var->tipo);
            continue;
        }

        if(atual.simbolo == TOKEN_SABRE_PARENTESES) {
            pilha[topo_pilha++] = atual;
            continue;
        }

        if(atual.simbolo == TOKEN_SFECHA_PARENTESES) {
            while(topo_pilha > 0 && pilha[topo_pilha-1].simbolo != TOKEN_SABRE_PARENTESES) {
                Token op = pilha[--topo_pilha];
                exp_posfix.tokens[exp_posfix.quantidade++] = op;

                // Verifica tipos para o operador
                if(eh_operador(op.simbolo)) {
                    char tipo2[20], tipo1[20];
                    strcpy(tipo2, tipos_operandos[--num_tipos]);
                    strcpy(tipo1, tipos_operandos[--num_tipos]);

                    char* tipo_resultado = verifica_tipo_operacao(op, tipo1, tipo2);
                    if(tipo_resultado == NULL) return;

                    strcpy(tipos_operandos[num_tipos++], tipo_resultado);
                }
            }

            if(topo_pilha > 0) {
                topo_pilha--; // Remove o abre parênteses
            } else {
                printf("\nErro: Parênteses não balanceados!\n");
                erro();
                return;
            }
            continue;
        }

        if(eh_operador(atual.simbolo)) {
            bool eh_unario = eh_operador_unario(lista_expressao.tokens, i);

            if(eh_unario) {
                if(atual.simbolo != TOKEN_SMAIS && atual.simbolo != TOKEN_SMENOS) {
                    printf("\nErro: Operador unário inválido!\n");
                    erro();
                    return;
                }
                // Operadores unários só podem ser aplicados a inteiros
                if(num_tipos > 0 && strcmp(tipos_operandos[num_tipos-1], "inteiro") != 0) {
                    printf("\nErro: Operador unário só pode ser aplicado a inteiros!\n");
                    erro();
                    return;
                }
            } else {
                while(topo_pilha > 0 &&
                      pilha[topo_pilha-1].simbolo != TOKEN_SABRE_PARENTESES &&
                      precedencia(pilha[topo_pilha-1], false) >= precedencia(atual, eh_unario)) {
                    Token op = pilha[--topo_pilha];
                    exp_posfix.tokens[exp_posfix.quantidade++] = op;

                    if(eh_operador(op.simbolo)) {
                        char tipo2[20], tipo1[20];
                        strcpy(tipo2, tipos_operandos[--num_tipos]);
                        strcpy(tipo1, tipos_operandos[--num_tipos]);

                        char* tipo_resultado = verifica_tipo_operacao(op, tipo1, tipo2);
                        if(tipo_resultado == NULL) return;

                        strcpy(tipos_operandos[num_tipos++], tipo_resultado);
                    }
                }
            }
            pilha[topo_pilha++] = atual;
        }
    }

    // Desempilha operadores restantes
    while(topo_pilha > 0) {
        if(pilha[topo_pilha-1].simbolo == TOKEN_SABRE_PARENTESES) {
            printf("\nErro: Parênteses não balanceados!\n");
            erro();
            return;
        }
        Token op = pilha[--topo_pilha];
        exp_posfix.tokens[exp_posfix.quantidade++] = op;

        if(eh_operador(op.simbolo)) {
            char tipo2[20], tipo1[20];
            strcpy(tipo2, tipos_operandos[--num_tipos]);
            strcpy(tipo1, tipos_operandos[--num_tipos]);

            char* tipo_resultado = verifica_tipo_operacao(op, tipo1, tipo2);
            if(tipo_resultado == NULL) return;

            strcpy(tipos_operandos[num_tipos++], tipo_resultado);
        }
    }

    // O tipo final da expressão é o último tipo calculado
    if(num_tipos > 0) {
        strcpy(exp_posfix.tipo, tipos_operandos[num_tipos-1]);
    }

    lista_expressao.quantidade = 0;
    lista_expressao.em_analise = false;
    strcpy(lista_expressao.tipo, "");
    memset(lista_expressao.tokens, 0, sizeof(Token) * 100);
    imprime_lista_tokens(&exp_posfix, "Lista Posfix Final");

}

char* verifica_tipo_operacao(Token operador, char* tipo1, char* tipo2) {
    printf("\nVerificando operação: %s entre tipos: %s e %s\n",
           operador.lexema, tipo1, tipo2);

    // Se for identificador, buscar na tabela de símbolos
    if(tipo1[0] == '\0' || tipo2[0] == '\0') {
        printf("\nErro: Tipo não definido para algum operando\n");
        erro();
        return NULL;
    }

    // Se algum dos tipos for função, usar o tipo de retorno
    if(strstr(tipo1, "funcao") != NULL) {
        if(strstr(tipo1, "funcao inteiro") != NULL) {
            printf("\nConvertendo %s para inteiro\n", tipo1);
            tipo1 = "inteiro";
        }
        else if(strstr(tipo1, "funcao booleana") != NULL) {
            printf("\nConvertendo %s para booleano\n", tipo1);
            tipo1 = "booleano";
        }
    }
    if(strstr(tipo2, "funcao") != NULL) {
        if(strstr(tipo2, "funcao inteiro") != NULL) {
            printf("\nConvertendo %s para inteiro\n", tipo2);
            tipo2 = "inteiro";
        }
        else if(strstr(tipo2, "funcao booleana") != NULL) {
            printf("\nConvertendo %s para booleano\n", tipo2);
            tipo2 = "booleano";
        }
    }

    // Se for procedimento, erro
    if(strstr(tipo1, "procedimento") != NULL || strstr(tipo2, "procedimento") != NULL) {
        printf("\nErro: Não é possível realizar operações com procedimentos\n");
        erro();
        return NULL;
    }

    switch(operador.simbolo) {
        case TOKEN_SMAIS:
        case TOKEN_SMENOS:
        case TOKEN_SMULT:
        case TOKEN_SDIV:
            if(strcmp(tipo1, "inteiro") == 0 && strcmp(tipo2, "inteiro") == 0)
                return "inteiro";
            printf("\nErro: Operação %s requer operandos inteiros! (recebeu %s e %s)\n",
                   operador.lexema, tipo1, tipo2);
            erro();
            return NULL;

        // Resto igual...
    }
    return NULL;
}

void Analisa_chamada_funcao(){
    printf("\nIdentificador Aceito\n");
    token = LerProximoToken(arquivo, &caractere); // +
}

void Analisa_chamada_de_procedimento(){
    printf("\n\nChamada de Procedimento\n\n");
}

void Analisa_fator(){
    printf("\n\n estamos no analisa fator \n\n");
    if(token.simbolo == TOKEN_IDENTIFICADOR){
        adiciona_token_expressao(token);  // Adiciona identificador
        Node* resultado = pesquisa_tabela(tabelaSimbolos, token.lexema, nivel);

        imprimirTabela(tabelaSimbolos);

        if(resultado != NULL) { // Se encontrou na tabela
            if(strcmp(resultado->tipo, "funcao inteiro") == 0 ||
               strcmp(resultado->tipo, "funcao booleana") == 0) {
                Analisa_chamada_funcao();
            } else {
                token = LerProximoToken(arquivo, &caractere);
            }
        } else {
            printf("\nErro: Identificador não encontrado na tabela de símbolos\n"); sleep(1);
            erro();
        }
    }
    else{
        if(token.simbolo == TOKEN_DIGITO){
            adiciona_token_expressao(token);  // Adiciona dígito
            printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
            token = LerProximoToken(arquivo, &caractere);
            printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        }
        else{
            if(token.simbolo == TOKEN_SNAO){
                adiciona_token_expressao(token);  // Adiciona operador não
                token = LerProximoToken(arquivo, &caractere);
                Analisa_fator();
            }
            else{
                if(token.simbolo == TOKEN_SABRE_PARENTESES){
                    adiciona_token_expressao(token);  // Adiciona abre parênteses
                    token = LerProximoToken(arquivo, &caractere);
                    Analisa_expressao(); // entra aqui com a
                    if(token.simbolo == TOKEN_SFECHA_PARENTESES){
                        adiciona_token_expressao(token);  // Adiciona fecha parênteses
                        token = LerProximoToken(arquivo, &caractere);
                        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
                    }
                    else{
                        printf("\n\nEra esperado um fecha parenteses\n\n");
                        erro();
                    }
                }
                else{
                    if(token.lexema == "verdadeiro" || token.lexema == "falso"){
                        adiciona_token_expressao(token);  // Adiciona booleano
                        token = LerProximoToken(arquivo, &caractere);
                    }
                    else{
                        printf("\n\nEra esperado um 'verdadeiro' ou 'falso'\n\n");
                        erro();
                    }
                }
            }
        }
    }
}

void Analisa_termo(){
    Analisa_fator();
    while(token.simbolo == TOKEN_SMULT || token.simbolo == TOKEN_SDIV || token.simbolo == TOKEN_SE){
        adiciona_token_expressao(token);  // Adiciona operador
        token = LerProximoToken(arquivo, &caractere);
        Analisa_fator();
    }
}

void Analisa_expressao_simples(){
    if(token.simbolo == TOKEN_SMAIS || token.simbolo == TOKEN_SMENOS){
        adiciona_token_expressao(token);  // Adiciona operador unário
        token = LerProximoToken(arquivo, &caractere);
    }
    Analisa_termo();
    while(token.simbolo == TOKEN_SMAIS || token.simbolo == TOKEN_SMENOS || token.simbolo == TOKEN_SOU){
        adiciona_token_expressao(token);  // Adiciona operador
        token = LerProximoToken(arquivo, &caractere);
        Analisa_termo();
    }
}

void Analisa_expressao(){
    Analisa_expressao_simples();
    if(token.simbolo == TOKEN_SMAIOR || token.simbolo == TOKEN_SMAIOR_IGUAL ||
       token.simbolo == TOKEN_SIGUAL || token.simbolo == TOKEN_SMENOR ||
       token.simbolo == TOKEN_SMENOR_IGUAL || token.simbolo == TOKEN_SDIFERENTE){
        adiciona_token_expressao(token);  // Adiciona operador relacional
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        token = LerProximoToken(arquivo, &caractere);
        Analisa_expressao_simples();
    }
}

void Analisa_declaracao_funcao() {
   char auxiliar[100];
   token = LerProximoToken(arquivo, &caractere);
   nivel = 1;

   if (token.simbolo == TOKEN_IDENTIFICADOR) {
       printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
       imprimirTabela(tabelaSimbolos);
       Node* var = pesquisa_declfunc_tabela(tabelaSimbolos, token.lexema); //

       if (var == NULL) { // nao encontrou duplicado nome na tabela
            printf("\n\nNão encontrou nome duplicado na tabela\n\n");
           strcpy(auxiliar, token.lexema); //auxiliar recebe o nome que vai pra tabela
           inserirNaTabela(&tabelaSimbolos, token.lexema, nivel, "funcao", 0);
           token = LerProximoToken(arquivo, &caractere);

           if (token.simbolo == TOKEN_SDOISPONTOS) {
               token = LerProximoToken(arquivo, &caractere);

               if (token.simbolo == TOKEN_SINTEIRO || token.simbolo == TOKEN_SBOOLEANO) {
                   if(token.simbolo == TOKEN_SINTEIRO){
                        TABSIMB(tabelaSimbolos, auxiliar, 0);
                        imprimirTabela(tabelaSimbolos);
                   }
                   else{
                        TABSIMB(tabelaSimbolos, auxiliar, 1);
                        imprimirTabela(tabelaSimbolos);
                   }
                   token = LerProximoToken(arquivo, &caractere);

                   if (token.simbolo == TOKEN_SPONTO_VIRGULA) {
                        printf("\n\nPassou o ponto e virgula da função\n\n");
                       Analisa_bloco();
                   }
               } else {
                   printf("\n\nEra esperado um Inteiro ou um Booleano\n\n");
                   erro();
               }
           } else {
               printf("\n\nEra esperado um Dois Pontos\n\n");
               erro();
           }
       } else {
           printf("\n\nJá existe um elemento na tabela com o nome desta função\n\n");
           erro();
       }
   } else {
       printf("\n\nEra esperado um identificador\n\n");
       erro();
   }
   Desempilha(&tabelaSimbolos, nivel);
}

void Analisa_declaracao_procedimento() {
   token = LerProximoToken(arquivo, &caractere);
   nivel = 1;

   if (token.simbolo == TOKEN_IDENTIFICADOR) {
       Node* var = pesquisa_declproc_tabela(tabelaSimbolos, token.lexema);

       if (var == NULL) { // se o proc é novo na tabela
           inserirNaTabela(&tabelaSimbolos, token.lexema, nivel, "procedimento", 0);
           token = LerProximoToken(arquivo, &caractere);

           if (token.simbolo == TOKEN_SPONTO_VIRGULA) {
               Analisa_bloco();
           } else {
               printf("\n\nEra esperado um ponto e virgula\n\n");
               erro();
           }
       } else {
           printf("\n\nO nome deste procedimento já foi utilizado no código\n\n");
           erro();
       }
   } else {
       printf("\n\nEra esperado um identificador\n\n");
       erro();
   }
   Desempilha(&tabelaSimbolos, nivel);
}

void Analisa_se(){
    token = LerProximoToken(arquivo, &caractere);
    printf("Analisador Léxico encontrou: Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
    Analisa_expressao();
    converte_para_posfix();
    limpa_lista_expressao();
    if(token.simbolo == TOKEN_SENTAO){
        token = LerProximoToken(arquivo, &caractere);
        Analisa_comando_simples();
        if(token.simbolo == TOKEN_SSENAO){
            token = LerProximoToken(arquivo, &caractere);
            Analisa_comando_simples();
        }
    }
    else{
            printf("\n\nEra esperado um Token Entao\n\n"); sleep(3);
        erro();
    }
}

void Analisa_enquanto(){
    token = LerProximoToken(arquivo, &caractere);
    printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
    Analisa_expressao(); // chamada com (
    converte_para_posfix();
    if(token.simbolo == TOKEN_SFACA){
        token = LerProximoToken(arquivo, &caractere);
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        Analisa_comando_simples();
    }
    else{
            printf("\n\nEra pra ser token faça\n\n");
        erro();
    }
}

void Analisa_escreva(){
    printf("\n\nEntrou no escreva\n\n");
    token = LerProximoToken(arquivo, &caractere);
    if(token.simbolo == TOKEN_SABRE_PARENTESES){
        token = LerProximoToken(arquivo, &caractere);
        if(token.simbolo == TOKEN_IDENTIFICADOR){
            Node* var = pesquisa_declvarfunc_tabela(tabelaSimbolos, token.lexema); // parecido com o leia, mas ignora pro tipo
            if(var != NULL){ // se existir alguma entrada na tabela que tenha token.lexema
                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo == TOKEN_SFECHA_PARENTESES){
                    token = LerProximoToken(arquivo, &caractere);
                }
                else{
                        printf("\n\nEra esperado um fecha parenteses\n\n");
                    erro();
                }
            }
            else{
                printf("\n\nO identificador não está presente na tabela\n\n");
                erro();
            }
        }
        else{
            printf("\n\nEra esperado um identificador\n\n");
            erro();
        }
    }
    else{
        printf("\n\nEra esperado um abre parenteses\n\n");
        erro();
    }
}

void Analisa_leia(){

    token = LerProximoToken(arquivo, &caractere);
    printf("\n\n Entrou no leia\n\n");
    if(token.simbolo == TOKEN_SABRE_PARENTESES){
        token = LerProximoToken(arquivo, &caractere);
        if(token.simbolo == TOKEN_IDENTIFICADOR){
            Node* var = pesquisa_declvar_tabela(tabelaSimbolos, token.lexema);
            if(var != NULL){
                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo == TOKEN_SFECHA_PARENTESES){
                        printf("\n\nFechou o parenteses do analisa leia\n\n");
                    token = LerProximoToken(arquivo, &caractere);
                }
                else{
                        printf("\n\nEra esperado um fecha parenteses\n\n");
                    erro();
                }
            }
            else{
                printf("\n\nO identificador não está presente na tabela ou  é do tipo errado\n\n");
                erro();
            }
        }
        else{
                printf("\n\nEra esperado um identificador\n\n");
            erro();
        }
    }
    else{
            printf("\n\nEra esperado um abre parenteses\n\n");
        erro();
    }
}

void Analisa_atribuicao(){
    token = LerProximoToken(arquivo, &caractere); // x;
    Analisa_expressao();
    converte_para_posfix();
}

void Analisa_atrib_chprocedimento(){
    token = LerProximoToken(arquivo, &caractere); // :=
    if(token.simbolo == TOKEN_ATRIBUICAO){
        Analisa_atribuicao();
    }
    else{
        Analisa_chamada_de_procedimento();
    }
}

void Analisa_comando_simples(){
    printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
    if(token.simbolo == TOKEN_IDENTIFICADOR){
        Analisa_atrib_chprocedimento();
    }
    else if(token.simbolo == TOKEN_SSE){
        Analisa_se();
    }
    else if(token.simbolo == TOKEN_SENQUANTO){
        Analisa_enquanto();
    }
    else if(token.simbolo == TOKEN_SLEIA){
        Analisa_leia();
    }
    else if(token.simbolo == TOKEN_SESCREVA){
        Analisa_escreva();
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
    }
    else{
        Analisa_comandos();
    }
}

void Analisa_comandos(){
    if(token.simbolo == TOKEN_SINICIO){
        token = LerProximoToken(arquivo, &caractere); // nesse ponto vale a
        Analisa_comando_simples();
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        //printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        while(token.simbolo != TOKEN_SFIM){
            if(token.simbolo == TOKEN_SPONTO_VIRGULA){
                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo != TOKEN_SFIM){
                    Analisa_comando_simples();
                    printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
                }
            }
            else{
                    printf("\n\nEra esperado um ponto e vírgula\n\n");
                erro();
                return;
            }
        }
        token = LerProximoToken(arquivo, &caractere);
    }
    else{
            printf("\n\nEra esperado um Token Início ou uma nova subrotina / função\n\n");
        erro();
    }
}

void Analisa_tipo(){
    if(token.simbolo != TOKEN_SINTEIRO && token.simbolo != TOKEN_SBOOLEANO){
            printf("\n\nEra esperado um inteiro ou um Booleano\n\n");
        erro();
    }
    else{
        colocarTipoNasVariaveis(&tabelaSimbolos, token.lexema);
        printf("\nEntrou no Analisa_Tipo");
        imprimirTabela(tabelaSimbolos);
    }
    token = LerProximoToken(arquivo, &caractere);
}

void Analisa_subrotinas(){
    while(token.simbolo == TOKEN_SPROCEDIMENTO || token.simbolo == TOKEN_SFUNCAO){
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        if(token.simbolo == TOKEN_SPROCEDIMENTO){
            Analisa_declaracao_procedimento();
        }
        else{
            Analisa_declaracao_funcao();
        }
        if(token.simbolo == TOKEN_SPONTO_VIRGULA){
            token = LerProximoToken(arquivo, &caractere);
        }
        else{
            printf("\n\nEra esperado um ponto e virgula (foi esse!!!!!!!!!!)\n\n");
            erro();
        }
    }
}

void Analisa_variaveis(){
    do{
        if(token.simbolo == TOKEN_IDENTIFICADOR){
            if(!Pesquisa_duplicvar_tabela(tabelaSimbolos, token.lexema)){ // se tiver duplicação na tabela, não entra no if
                inserirNaTabela(&tabelaSimbolos, token.lexema, 0, "variavel", 0);
                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo == TOKEN_SVIRGULA || token.simbolo == TOKEN_SDOISPONTOS){
                    if(token.simbolo == TOKEN_SVIRGULA){
                        token = LerProximoToken(arquivo, &caractere);
                        if(token.simbolo == TOKEN_SDOISPONTOS){
                            printf("\n\nIdentificado um token Dois Pontos\n\n");
                            erro();
                        }
                    }
                }
                else{
                    printf("\n\nEra esperado uma Vírgula ou um Dois Pontos\n\n");
                    erro();
                    break;
                }
            }
            else{
                printf("\n\nJá está sendo usado este identificador\n\n");
                erro();
                break;
            }
        }
        else{
                printf("\n\nEra esperado um identificador\n\n");
            erro();
            break;
        }
    }while(token.simbolo != TOKEN_SDOISPONTOS);
    token = LerProximoToken(arquivo, &caractere);
    Analisa_tipo();
}

void Analisa_et_variaveis(){
    if(token.simbolo == TOKEN_SVAR){
        token = LerProximoToken(arquivo, &caractere);
        if(token.simbolo == TOKEN_IDENTIFICADOR){
            while(token.simbolo == TOKEN_IDENTIFICADOR){
                Analisa_variaveis();
                if(token.simbolo == TOKEN_SPONTO_VIRGULA){
                    token = LerProximoToken(arquivo, &caractere);
                }
                else{
                        printf("\n\nEra esperado um ponto e vírgula\n\n");
                    erro();
                    break;
                }
            }
        }
        else{
                printf("\n\nEra esperado um identificador\n\n");
            erro();
        }
    }
}

void Analisa_bloco(){
    token = LerProximoToken(arquivo, &caractere);
    printf("Analisador Léxico encontrou: Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
    Analisa_et_variaveis();
    Analisa_subrotinas();
    Analisa_comandos();
}

void AnalisadorSintatico() {

   if(token.simbolo == TOKEN_SPROGRAMA) {
       token = LerProximoToken(arquivo, &caractere);
       printf("Analisador Léxico encontrou: Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);

       if(token.simbolo == TOKEN_IDENTIFICADOR) {
           inserirNaTabela(&tabelaSimbolos, token.lexema, 0, "nomedeprograma", 0); // insere_tabela ok
           imprimirTabela(tabelaSimbolos);
           token = LerProximoToken(arquivo, &caractere);
           if(token.simbolo == TOKEN_SPONTO_VIRGULA) {
               Analisa_bloco();
               printf("\n\nSaiu do analisa bloco\n\n");
               printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);

               if(token.simbolo == TOKEN_SPONTO) {
                   token = LerProximoToken(arquivo, &caractere);
                   if(token.simbolo == TOKEN_ERRO && strcmp(token.lexema, "EOF") == 0) {
                       printf("\nEncerrado\n");
                   } else {
                       printf("Erro no finalzinho, ultimo if");
                       erro();
                   }
               } else {
                   printf("Provavelmente erro no analisa bloco, falhou na entrada de token ponto (pós fim)");
                   erro();
               }
           } else {
               printf("Erro no ponto e virgula");
               erro();
           }
       } else {
           printf("Erro no identificador");
           erro();
       }
   } else {
       printf("Erro no token programa");
       erro();
   }
}

void erro() {
    char mensagem[256];

    #ifdef GUI_MODE
        // No modo GUI, formata a mensagem para a listbox
        if(token.simbolo != TOKEN_ERRO) {
            sprintf(mensagem, "Erro na linha %d - Token atual: %s (%s)",
                    linhas, token.lexema, Nomedotoken[token.simbolo]);
        } else {
            sprintf(mensagem, "Erro na linha %d - Token inválido", linhas);
        }
        // Envia para a listbox
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)mensagem);
    #else
        // No modo console, mantém o comportamento original
        printf("\n\nErro!\n\n");
        printf("\n\nErro na linha: %d\n\n", linhas);
    #endif
}

void contador_linhas(FILE *arquivo, int caractere, int linhas){ //contador de linhas do programa
    arquivo = fopen("teste.txt", "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return 1;
    }

    // Reiniciar o ponteiro do arquivo para o início
    rewind(arquivo);

    // Ler o arquivo desde o início novamente
    caractere = LerCaracter(arquivo);
    linhas=0; // reseta o contador
    while (caractere != EOF) {
        if(caractere == '\n'){
            linhas++;
        }

        caractere = LerCaracter(arquivo);
    }

    fclose(arquivo);

    printf("\n\n%d linhas de Codigo\n\n", linhas+1);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {  // Se tiver argumentos, usa modo console
        linhas = 1;
        setlocale(LC_ALL, "Portuguese");
        arquivo = fopen("teste.txt", "r");
        if (arquivo == NULL) {
            printf("Erro ao abrir o arquivo.\n");
            return 1;
        }

        bool continuar = true;
        caractere = LerCaracter(arquivo);

        while (continuar) {
            token = LerProximoToken(arquivo, &caractere);
            if (token.simbolo != TOKEN_ERRO) {
                printf("Analisador Léxico encontrou: Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
                AnalisadorSintatico();  // Envia o token para o analisador sintático
            } else if (strcmp(token.lexema, "EOF") == 0) {
                printf("Fim do arquivo alcançado.\n");
                continuar = false;
            } else {
                printf("Erro: Token inválido encontrado.\n");
            }

            printf("\nPressione Enter para continuar ou 'q' para sair: ");
            char input = getchar();
            while (getchar() != '\n');  // Limpa o buffer de entrada
            if (input == 'q' || input == 'Q') {
                continuar = false;
            }
        }

        fclose(arquivo);
        contador_linhas(arquivo, caractere, linhas);
        return 0;
    }
    else {  // Se não tiver argumentos, inicia modo GUI
        #ifdef _WIN32
            return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
        #else
            printf("Modo GUI só está disponível no Windows!\n");
            return 1;
        #endif
    }
}

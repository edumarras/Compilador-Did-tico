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
#define STATUSCLASSNAME "msctls_statusbar32"
#define ID_STATUSBAR 8  // Adicionamos um ID específico para a barra de status
#define MAX_LEXEMA 100
#define ID_EDIT_BG_TIMER 9
#define ID_HIGHLIGHT_TIMER 9
#define HIGHLIGHT_DURATION 2000  // 2 segundos de destaque

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

const char *Nomedotoken[] = {
    "TOKEN_DIGITO",
    "TOKEN_IDENTIFICADOR",
    "TOKEN_PALAVRA_RESERVADA",
    "TOKEN_ATRIBUICAO",
    "TOKEN_OPERADOR_ARITMETICO",
    "TOKEN_OPERADOR_RELACIONAL",
    "TOKEN_PONTUACAO",
    "TOKEN_ERRO",
    "TOKEN_SPROGRAMA",
    "TOKEN_SSE",
    "TOKEN_SENTAO",
    "TOKEN_SSENAO",
    "TOKEN_SENQUANTO",
    "TOKEN_SFACA",
    "TOKEN_SINICIO",
    "TOKEN_SFIM",
    "TOKEN_SESCREVA",
    "TOKEN_SLEIA",
    "TOKEN_SVAR",
    "TOKEN_SINTEIRO",
    "TOKEN_SBOOLEANO",
    "TOKEN_SVERDADEIRO",
    "TOKEN_SFALSO",
    "TOKEN_SPROCEDIMENTO",
    "TOKEN_SFUNCAO",
    "TOKEN_SDIV",
    "TOKEN_SE",
    "TOKEN_SOU",
    "TOKEN_SNAO",
    "TOKEN_SPONTO_VIRGULA",
    "TOKEN_SVIRGULA",
    "TOKEN_SABRE_PARENTESES",
    "TOKEN_SFECHA_PARENTESES",
    "TOKEN_SPONTO",
    "TOKEN_SMAIS",
    "TOKEN_SMENOS",
    "TOKEN_SMULT",
    "TOKEN_SERRO",
    "TOKEN_SATRIB",
    "TOKEN_SDOISPONTOS",
    "TOKEN_SMAIOR",
    "TOKEN_SMENOR",
    "TOKEN_SIGUAL",
    "TOKEN_SDIFERENTE",
    "TOKEN_SMAIOR_IGUAL",
    "TOKEN_SMENOR_IGUAL"
};

typedef struct MemorySlot {
    int start;
    int size;
    struct MemorySlot* next;
} MemorySlot;

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
    Token* tokens;
    int tamanho;
    int capacidade;
    bool em_processamento;
    char tipo_resultado[20];
} Expressao;

typedef struct {
    Token* items;
    int topo;
    int capacidade;
} Pilha;

// Estrutura para armazenar informações sobre o destaque
typedef struct {
    HWND hwndEdit;
    int linha;
    COLORREF corOriginal;
    bool ativo;
} DestaqueInfo;

typedef struct {
    char tipos[100][20];  // Array para armazenar os tipos
    int topo;
} TipoStack;

DestaqueInfo destaque = {0};

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
bool erro_encontrado = false;
int rotulo_geracao = 2;  // Contador de rótulos
int end = 1;     // Contador de endereços de memória
FILE* arquivo_obj = NULL;
MemorySlot* free_memory = NULL;

Expressao* expressao_atual = NULL; // expressao pósfix

// Protótipos da GUI
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CompiladorGUI(HWND hwnd);
void CriarMenu(HWND hwnd);
void CriarAreaTexto(HWND hwnd);
void MoverCursorParaErro(int linha, int coluna);
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
void Analisa_chamada_funcao(void);
void Analisa_chamada_de_procedimento(char novo[50]);
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
void Analisa_atribuicao(char *aux);
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
Node* pesquisa_tipo_tabela(Node* top, const char* lexema, int nivel);

//funções para a geração de código
void GERA(const char* rot, const char* inst, const char* p3, const char* p4);
char* int_to_str(int num);
void finaliza_codigo();

//funções para a pilha

Pilha* criar_pilha(int capacidade);
bool pilha_vazia(Pilha* p);
void empilhar(Pilha* p, Token t);
Token desempilhar(Pilha* p);
Token topo_pilha(Pilha* p);
const char* tipo_resultado(const char* tipo1, const char* tipo2, TipoToken operador);

//funções para manipular a expressão

Expressao* criar_expressao(int capacidade);
void adicionar_token(Expressao* exp, Token t);
void imprimir_expressao(Expressao* exp, const char* descricao);

//conversão pós-fixa

bool eh_operador_unario(Token* tokens, int pos);
int precedencia(Token token, bool unario);
Expressao* converter_para_posfixa(Expressao* entrada);

void CompiladorGUI(HWND hwnd) {
    if (!hwnd) {
        MessageBox(NULL, "Invalid window handle", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    debug_print("=== Iniciando Compilação ===");
    erro_encontrado = false;

    // Reset global state
    if (arquivo_obj != NULL) {
        fclose(arquivo_obj);
        arquivo_obj = NULL;
    }

    if (expressao_atual != NULL) {
        free(expressao_atual->tokens);
        free(expressao_atual);
        expressao_atual = NULL;
    }

    // Clear existing table
    while (tabelaSimbolos != NULL) {
        Node* temp = tabelaSimbolos;
        tabelaSimbolos = tabelaSimbolos->next;
        free(temp);
    }

    // Reset compilation state
    linhas = 1;
    nivel = 0;
    end = 1;
    rotulo_geracao = 1;

   debug_print("=== Iniciando Compilação ===");
   erro_encontrado = false;
   SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

   int tamanho = GetWindowTextLength(hEdit);
   if(tamanho <= 0) {
       MessageBox(hwnd, "Editor está vazio!", "Aviso", MB_OK | MB_ICONINFORMATION);
       return;
   }

   char* texto = (char*)malloc(tamanho + 1);
   if(texto == NULL) {
       MessageBox(hwnd, "Erro de memória!", "Erro", MB_OK | MB_ICONERROR);
       return;
   }

   // Pega o caminho do executável para criar os arquivos no mesmo diretório
   char path[MAX_PATH];
   GetModuleFileName(NULL, path, MAX_PATH);
   char* last_slash = strrchr(path, '\\');
   if (last_slash != NULL) {
       *(last_slash + 1) = '\0';  // Mantém apenas o caminho, remove o nome do exe
   }

   // Cria o caminho completo para os arquivos
   char temp_path[MAX_PATH];
   char obj_path[MAX_PATH];
   sprintf(temp_path, "%stemp_compile.txt", path);
   sprintf(obj_path, "%sPROGRAMA.OBJ", path);

   debug_print("Obtendo texto do editor...");
   GetWindowText(hEdit, texto, tamanho + 1);

   FILE* temp = fopen(temp_path, "w");
   if(temp == NULL) {
       MessageBox(hwnd, "Erro ao criar arquivo temporário!", "Erro", MB_OK | MB_ICONERROR);
       free(texto);
       return;
   }

   debug_print("Criando arquivo temporário...");
   fputs(texto, temp);
   fclose(temp);
   free(texto);

   linhas = 1;
   nivel = 0;
   tabelaSimbolos = NULL;
   arquivo_obj = NULL;  // Reseta o arquivo_obj

   debug_print("Abrindo arquivo para compilação...");
   arquivo = fopen(temp_path, "r");
   if(arquivo == NULL) {
       MessageBox(hwnd, "Erro ao abrir arquivo temporário!", "Erro", MB_OK | MB_ICONERROR);
       return;
   }

   caractere = LerCaracter(arquivo);
   token = LerProximoToken(arquivo, &caractere);
   if(token.simbolo != TOKEN_ERRO) {
       debug_print("Iniciando análise sintática...");
       AnalisadorSintatico();
       if (erro_encontrado) {  // <-- ADICIONE AQUI
           // Limpa recursos e retorna imediatamente
           if(arquivo_obj != NULL) {
               debug_print("Fechando arquivo objeto...");
               finaliza_arquivo_obj();
           }
           if(arquivo != NULL) {
               debug_print("Fechando arquivo temporário...");
               fclose(arquivo);
           }
           debug_print("Removendo arquivo temporário...");
           remove(temp_path);
           debug_print("=== Compilação encerrada por erro ===");
           UpdateWindow(hwnd);
           return;
       }
       if (!erro_encontrado) {
           debug_print("Compilação concluída com sucesso!");
           SetWindowText(hStatus, "Compilação concluída com sucesso!");
           SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Programa compilado com sucesso!");
       }
   } else {
       debug_print("Erro na análise léxica!");
       SetWindowText(hStatus, "Erro na análise léxica!");
       SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Erro: Token inválido encontrado na análise léxica.");
   }

   debug_print("Finalizando compilação...");

   // Fecha os arquivos
   if(arquivo_obj != NULL) {
       debug_print("Fechando arquivo objeto...");
       finaliza_arquivo_obj();
   }

   if(arquivo != NULL) {
       debug_print("Fechando arquivo temporário...");
       fclose(arquivo);
   }

   debug_print("Removendo arquivo temporário...");
   remove(temp_path);

   debug_print("=== Fim da Compilação ===");
   UpdateWindow(hwnd);
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
        "EDIT",    // Mantemos o EDIT padrão
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


// Timer proc para remover o destaque
VOID CALLBACK RemoverDestaque(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    if (destaque.ativo) {
        // Remove a seleção
        SendMessage(destaque.hwndEdit, EM_SETSEL, -1, -1);
        destaque.ativo = false;

        // Remove o timer
        KillTimer(hwnd, ID_HIGHLIGHT_TIMER);

        // Força redesenho
        InvalidateRect(destaque.hwndEdit, NULL, TRUE);
    }
}

// Função modificada para destacar a linha com erro
void MoverCursorParaErro(int linha, int coluna) {
    int inicio, fim;
    EncontrarPosicoesLinha(linha, &inicio, &fim);

    // Configura o destaque
    destaque.hwndEdit = hEdit;
    destaque.linha = linha;
    destaque.ativo = true;

    // Seleciona a linha inteira
    SendMessage(hEdit, EM_SETSEL, inicio, fim);

    // Rola para tornar a seleção visível
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);

    // Dá foco ao editor
    SetFocus(hEdit);

    // Configura timer para remover o destaque
    SetTimer(GetParent(hEdit), ID_HIGHLIGHT_TIMER, HIGHLIGHT_DURATION, RemoverDestaque);
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
                        "Este compilador realiza análise léxica, sintática e semântica.\n\n"
                        "Carregue um arquivo de texto com um programa para executar, ou insira você mesmo o código no campo abaixo.\n",
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

                case ID_LISTBOX:
                    if(HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        // Pega o item selecionado
                        int idx = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                        if(idx != LB_ERR)
                        {
                            char texto[256];
                            SendMessage(hListBox, LB_GETTEXT, idx, (LPARAM)texto);

                            // Extrai linha e coluna do texto
                            int linha, coluna;
                            if(sscanf(texto, "%d:%d", &linha, &coluna) == 2)
                            {
                                MoverCursorParaErro(linha, coluna);
                            }
                        }
                    }
                    break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
            cleanup_tabela_simbolos();
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
    LoadLibrary("Msftedit.dll");  // Necessário para RichEdit 5.0

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

// Função para encontrar as posições de início e fim de uma linha
void EncontrarPosicoesLinha(int linha, int* inicio, int* fim) {
    int len = GetWindowTextLength(hEdit);
    char* buffer = (char*)malloc(len + 1);
    GetWindowText(hEdit, buffer, len + 1);

    int linhaAtual = 1;
    int pos = 0;
    *inicio = 0;

    // Encontra o início da linha
    while (pos < len && linhaAtual < linha) {
        if (buffer[pos] == '\n') {
            linhaAtual++;
            *inicio = pos + 1;
        }
        pos++;
    }

    // Encontra o fim da linha
    *fim = *inicio;
    while (*fim < len && buffer[*fim] != '\n' && buffer[*fim] != '\r') {
        (*fim)++;
    }

    free(buffer);
}

// FUNÇÕES PARA A GERAÇÃO DE CÓDIGO

void add_free_space(int start, int size) {
    MemorySlot* slot = malloc(sizeof(MemorySlot));
    slot->start = start;
    slot->size = size;
    slot->next = free_memory;
    free_memory = slot;
}

int find_free_space(int size) {
    // Se temos uma lista de espaços livres, usar o primeiro disponível
    if (free_memory != NULL) {
        int addr = free_memory->start;
        MemorySlot* temp = free_memory;
        free_memory = free_memory->next;
        free(temp);
        return addr;
    }

    // Se não há espaços livres, usar o próximo endereço disponível
    return end++;
}

void debug_print(const char* message) {
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)message);
}

void GERA(const char* rot, const char* inst, const char* p3, const char* p4) {
    // Para debug

    // Adicione este print no início da função
    printf("\nGerando instrução: [%s] [%s] [%s] [%s] chamado de %s linha %d",
           rot ? rot : "null",
           inst ? inst : "null",
           p3 ? p3 : "null",
           p4 ? p4 : "null",
           __FUNCTION__, __LINE__);

    static char full_path[MAX_PATH] = "";

    // Inicializa o arquivo na primeira chamada
    if (arquivo_obj == NULL) {
        char path[MAX_PATH];
        GetModuleFileName(NULL, path, MAX_PATH);
        char* last_slash = strrchr(path, '\\');
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
        }

        sprintf(full_path, "%sPROGRAMA.OBJ", path);

        char debug[512];
        sprintf(debug, "Tentando criar arquivo em: %s", full_path);
        debug_print(debug);

        arquivo_obj = fopen(full_path, "w");
        if (arquivo_obj == NULL) {
            char erro[256];
            sprintf(erro, "Erro ao criar arquivo. Errno: %d - %s", errno, strerror(errno));
            debug_print(erro);
            return;
        }

        debug_print("Arquivo criado com sucesso");
    }

    // Para debug
    char debug_write[256];
    sprintf(debug_write, "Escrevendo: [%s] [%s] [%s] [%s]",
            rot ? rot : "null",
            inst ? inst : "null",
            p3 ? p3 : "null",
            p4 ? p4 : "null");
    debug_print(debug_write);

    // Formata a saída com os espaçamentos corretos
    char formato[100];

    // Verifica se é um rótulo com 'L'
    if (rot && rot[0] == 'L') {
        // Remove o 'L' do rótulo e trata como número
        const char* numero_rotulo = rot + 1;  // Pula o 'L'
        if (inst && strcmp(inst, "HLT") == 0){
            sprintf(formato, "%%-%ds%%-%ds%%-%ds%%-%ds", 4, 8, 4, 4);
        }
        else{
            sprintf(formato, "%%-%ds%%-%ds%%-%ds%%-%ds\n", 4, 8, 4, 4);
        }
        if (fprintf(arquivo_obj, formato,
                   numero_rotulo,  // Usa apenas o número, sem o 'L'
                   inst ? inst : "",
                   p3 ? p3 : "",
                   p4 ? p4 : "") < 0) {
            char erro[256];
            sprintf(erro, "Erro ao escrever. Errno: %d - %s", errno, strerror(errno));
            debug_print(erro);
        }
    } else {
        if (inst && strcmp(inst, "HLT") == 0){
            sprintf(formato, "%%-%ds%%-%ds%%-%ds%%-%ds", 4, 8, 4, 4);
        }
        else{
            sprintf(formato, "%%-%ds%%-%ds%%-%ds%%-%ds\n", 4, 8, 4, 4);
        }
        if (fprintf(arquivo_obj, formato,
                   rot ? rot : "",
                   inst ? inst : "",
                   p3 ? p3 : "",
                   p4 ? p4 : "") < 0) {
            char erro[256];
            sprintf(erro, "Erro ao escrever. Errno: %d - %s", errno, strerror(errno));
            debug_print(erro);
        }
    }

    // Força a escrita no arquivo
    fflush(arquivo_obj);
}

// Função auxiliar para fechar o arquivo
void finaliza_arquivo_obj() {
    if (arquivo_obj != NULL) {
        debug_print("Fechando arquivo...");
        if (fclose(arquivo_obj) != 0) {
            char erro[256];
            sprintf(erro, "Erro ao fechar arquivo. Errno: %d - %s", errno, strerror(errno));
            debug_print(erro);
        } else {
            debug_print("Arquivo fechado com sucesso");
        }
        arquivo_obj = NULL;
    }
}

char* int_to_str(int num) {
    char* str = malloc(20);
    if (str == NULL) return NULL;
    snprintf(str, 20, "%d", num);
    return str;
}

//Funções para o pós fixo

// Função para inicializar a stack
TipoStack* criar_tipo_stack() {
    TipoStack* stack = (TipoStack*)malloc(sizeof(TipoStack));
    if (!stack) return NULL;
    stack->topo = -1;
    return stack;
}

void empilha_tipo(TipoStack* stack, const char* tipo) {
    if (!stack || stack->topo >= 99) return;
    stack->topo++;
    strncpy(stack->tipos[stack->topo], tipo, 19);
    stack->tipos[stack->topo][19] = '\0';
}

const char* desempilha_tipo(TipoStack* stack) {
    if (!stack || stack->topo < 0) return NULL;
    return stack->tipos[stack->topo--];
}

const char* topo_tipo(TipoStack* stack) {
    if (!stack || stack->topo < 0) return NULL;
    return stack->tipos[stack->topo];
}



Pilha* criar_pilha(int capacidade) {
    Pilha* p = (Pilha*)malloc(sizeof(Pilha));
    p->items = (Token*)malloc(sizeof(Token) * capacidade);
    p->topo = -1;
    p->capacidade = capacidade;
    return p;
}

bool pilha_vazia(Pilha* p) {
    return p->topo == -1;
}

void empilhar(Pilha* p, Token t) {
    if (p->topo < p->capacidade - 1) {
        p->items[++p->topo] = t;
        printf("\nDebug: Empilhou '%s'", t.lexema);
    }
}

Token desempilhar(Pilha* p) {
    Token t = p->items[p->topo--];
    printf("\nDebug: Desempilhou '%s'", t.lexema);
    return t;
}

Token topo_pilha(Pilha* p) {
    return p->items[p->topo];
}

// manipulação da expressão

Expressao* criar_expressao(int capacidade) {
    Expressao* exp = (Expressao*)malloc(sizeof(Expressao));
    if (!exp) return NULL;

    exp->tokens = (Token*)malloc(sizeof(Token) * capacidade);
    if (!exp->tokens) {
        free(exp);
        return NULL;
    }

    exp->tamanho = 0;
    exp->capacidade = capacidade;
    return exp;
}

void adicionar_token(Expressao* exp, Token t) {
    if (exp->tamanho < exp->capacidade) {
        exp->tokens[exp->tamanho++] = t;
        printf("\nDebug: Token adicionado: '%s' (%s)",
               t.lexema, Nomedotoken[t.simbolo]);
    }
}

void imprimir_expressao(Expressao* exp, const char* descricao) {
    printf("\n%s:", descricao);
    for (int i = 0; i < exp->tamanho; i++) {
        printf(" %s", exp->tokens[i].lexema);
    }
    printf("\n");
}

// suporte

bool eh_operador_unario(Token* tokens, int pos) {
    if (pos == 0) {
        return (tokens[pos].simbolo == TOKEN_SMAIS ||
                tokens[pos].simbolo == TOKEN_SMENOS);
    }

    TipoToken anterior = tokens[pos-1].simbolo;
    return (tokens[pos].simbolo == TOKEN_SMAIS || tokens[pos].simbolo == TOKEN_SMENOS) &&
           (anterior == TOKEN_SABRE_PARENTESES ||
            anterior == TOKEN_SMAIS ||
            anterior == TOKEN_SMENOS ||
            anterior == TOKEN_SMULT ||
            anterior == TOKEN_SDIV ||
            anterior == TOKEN_SE ||
            anterior == TOKEN_SOU ||
            anterior == TOKEN_SMAIOR ||
            anterior == TOKEN_SMENOR ||
            anterior == TOKEN_SIGUAL ||
            anterior == TOKEN_SDIFERENTE);
}

int precedencia(Token token, bool unario) {
   if (token.simbolo == TOKEN_SNAO) return 2;
   if (unario) return 5;
   switch(token.simbolo) {
       case TOKEN_SMULT:
       case TOKEN_SDIV:
           return 4;
       case TOKEN_SMAIS:
       case TOKEN_SMENOS:
           return 3;
       case TOKEN_SMAIOR:
       case TOKEN_SMENOR:
       case TOKEN_SIGUAL:
       case TOKEN_SDIFERENTE:
       case TOKEN_SMAIOR_IGUAL:
       case TOKEN_SMENOR_IGUAL:
           return 2;
       case TOKEN_SE:
           return 1;
       case TOKEN_SOU:
           return 0;
       default:
           return -1;
   }
}

// Modificamos obter_tipo_token para usar a nova função
const char* obter_tipo_token(Token token, Node* tabelaSimbolos) {
    if (token.simbolo == TOKEN_DIGITO) {
        return "inteiro";
    }

    if (token.simbolo == TOKEN_SVERDADEIRO || token.simbolo == TOKEN_SFALSO) {
        return "booleano";
    }

    if (token.simbolo == TOKEN_IDENTIFICADOR) {
        Node* var = pesquisa_tipo_tabela(tabelaSimbolos, token.lexema, nivel);
        if (var) {
            if (strcmp(var->tipo, "inteiro") == 0) {
                return "inteiro";
            }
            if (strcmp(var->tipo, "booleano") == 0) {
                return "booleano";
            }
            if (strcmp(var->tipo, "funcao inteiro") == 0) {
                return "inteiro";
            }
            if (strcmp(var->tipo, "funcao booleana") == 0) {
                return "booleano";
            }
        }
        return NULL;
    }

    return NULL;
}

bool tipos_compativeis(const char* tipo1, const char* tipo2, TipoToken operador) {
    const char* tipo_res = tipo_resultado(tipo1, tipo2, operador);

    // Se o resultado é válido e a expressão está sendo atribuída a uma variável booleana
    if (tipo_res != NULL && strcmp(tipo_res, "booleano") == 0) {
        return true;  // A expressão resulta em booleano, então é válida
    }

    return tipo_res != NULL;
}

const char* tipo_resultado(const char* tipo1, const char* tipo2, TipoToken operador) {
    static const char* TIPO_INTEIRO = "inteiro";
    static const char* TIPO_BOOLEANO = "booleano";

    // Operadores aritméticos
    if (operador == TOKEN_SMAIS || operador == TOKEN_SMENOS ||
        operador == TOKEN_SMULT || operador == TOKEN_SDIV) {
        if (strcmp(tipo1, TIPO_INTEIRO) == 0 && strcmp(tipo2, TIPO_INTEIRO) == 0) {
            return TIPO_INTEIRO;
        }
        return NULL;
    }

    // Operadores unários
    if (operador == TOKEN_SMAIS || operador == TOKEN_SMENOS) {
        char lexema_op[100];
        strcpy(lexema_op, Nomedotoken[operador]);
        if (strstr(lexema_op, "u") != NULL && strcmp(tipo2, TIPO_INTEIRO) == 0) {
            return TIPO_INTEIRO;
        }
        return NULL;
    }

    // Operadores relacionais - MODIFICADO: agora só aceita inteiros
    if (operador == TOKEN_SMAIOR || operador == TOKEN_SMENOR ||
        operador == TOKEN_SIGUAL || operador == TOKEN_SDIFERENTE ||
        operador == TOKEN_SMAIOR_IGUAL || operador == TOKEN_SMENOR_IGUAL) {
        if (strcmp(tipo1, TIPO_INTEIRO) == 0 && strcmp(tipo2, TIPO_INTEIRO) == 0) {
            return TIPO_BOOLEANO;
        }
        return NULL;  // Retorna NULL se tentar comparar booleanos
    }

    // Operadores lógicos
    if (operador == TOKEN_SE || operador == TOKEN_SOU) {
        if (strcmp(tipo1, TIPO_BOOLEANO) == 0 && strcmp(tipo2, TIPO_BOOLEANO) == 0) {
            return TIPO_BOOLEANO;
        }
        return NULL;
    }

    if (operador == TOKEN_SNAO) {
        if (strcmp(tipo2, TIPO_BOOLEANO) == 0) {
            return TIPO_BOOLEANO;
        }
        return NULL;
    }

    return NULL;
}

// conversão

Expressao* converter_para_posfixa(Expressao* entrada) {
    Pilha* operadores = criar_pilha(entrada->capacidade);
    Expressao* saida = criar_expressao(entrada->capacidade);

    for (int i = 0; i < entrada->tamanho; i++) {
        Token atual = entrada->tokens[i];

        // Operandos vão direto para a saída
        if (atual.simbolo == TOKEN_DIGITO ||
            atual.simbolo == TOKEN_IDENTIFICADOR ||
            atual.simbolo == TOKEN_SVERDADEIRO ||
            atual.simbolo == TOKEN_SFALSO) {
            adicionar_token(saida, atual);
        }
        // Abre parênteses vai para pilha
        else if (atual.simbolo == TOKEN_SABRE_PARENTESES) {
            empilhar(operadores, atual);
        }
        // Fecha parênteses desempilha até achar o abre parênteses
        else if (atual.simbolo == TOKEN_SFECHA_PARENTESES) {
            while (!pilha_vazia(operadores) &&
                   topo_pilha(operadores).simbolo != TOKEN_SABRE_PARENTESES) {
                adicionar_token(saida, desempilhar(operadores));
            }
            if (!pilha_vazia(operadores)) {
                desempilhar(operadores);  // Remove o abre parênteses
            }
        }
        // Operadores
        else {
            bool unario = eh_operador_unario(entrada->tokens, i);

            if (unario) {
                char novo_lexema[100];
                sprintf(novo_lexema, "%su", atual.lexema);
                strcpy(atual.lexema, novo_lexema);
                empilhar(operadores, atual);
            } else {
                while (!pilha_vazia(operadores) &&
                       topo_pilha(operadores).simbolo != TOKEN_SABRE_PARENTESES &&
                       precedencia(topo_pilha(operadores), false) >= precedencia(atual, false)) {
                    adicionar_token(saida, desempilhar(operadores));
                }
                empilhar(operadores, atual);
            }
        }
    }

    // Desempilha operadores restantes
    while (!pilha_vazia(operadores)) {
        Token op = desempilhar(operadores);
        if (op.simbolo != TOKEN_SABRE_PARENTESES) {
            adicionar_token(saida, op);
        }
    }

    return saida;
}

const char* analisar_tipos_posfixa(Expressao* exp) {
    static char tipos_stack[100][20];
    int tipos_top = -1;

    for (int i = 0; i < exp->tamanho; i++) {
        Token atual = exp->tokens[i];
        printf("\nProcessando token: %s", atual.lexema);

        if (atual.simbolo == TOKEN_DIGITO) {
            tipos_top++;
            strcpy(tipos_stack[tipos_top], "inteiro");
            printf("\nEmpilhou: %s", tipos_stack[tipos_top]);
            printf("\nTopo atual: %d", tipos_top);
            printf("\nTipo no topo: %s", tipos_stack[tipos_top]);
        }
        else if (atual.simbolo == TOKEN_SVERDADEIRO || atual.simbolo == TOKEN_SFALSO) {
            tipos_top++;
            strcpy(tipos_stack[tipos_top], "booleano");
            printf("\nTopo atual: %d", tipos_top);
            printf("\nTipo no topo: %s", tipos_stack[tipos_top]);
        }
        else if (atual.simbolo == TOKEN_IDENTIFICADOR) {
            tipos_top++;
            const char* tipo = obter_tipo_token(atual, tabelaSimbolos);
            if (!tipo) {
                erro();
                return NULL;
            }
            strcpy(tipos_stack[tipos_top], tipo);
            printf("\nTopo atual: %d", tipos_top);
            printf("\nTipo no topo: %s", tipos_stack[tipos_top]);
        }
        else {
            if (atual.simbolo == TOKEN_SNAO) {
                printf("\nAntes do NOT - Topo atual: %d", tipos_top);
                printf("\nAntes do NOT - Tipo no topo: %s", tipos_stack[tipos_top]);

                if (tipos_top < 0 || strcmp(tipos_stack[tipos_top], "booleano") != 0) {
                    erro();
                    return NULL;
                }
                strcpy(tipos_stack[tipos_top], "booleano");

                printf("\nDepois do NOT - Topo atual: %d", tipos_top);
                printf("\nDepois do NOT - Tipo no topo: %s", tipos_stack[tipos_top]);
            }
            else if (atual.simbolo == TOKEN_SE || atual.simbolo == TOKEN_SOU) {
                printf("\nAntes de op lógico - Topo atual: %d", tipos_top);
                printf("\nAntes de op lógico - Tipo no topo: %s", tipos_stack[tipos_top]);

                // Garante que há dois operandos na pilha
                if (tipos_top < 1) {
                    printf("\nErro: Operandos insuficientes para operação lógica");
                    erro();
                    return NULL;
                }

                // Verifica os dois últimos tipos
                if (tipos_top >= 1 && strcmp(tipos_stack[tipos_top], "booleano") == 0 &&
                    strcmp(tipos_stack[tipos_top-1], "booleano") == 0) {
                    tipos_top--;
                    strcpy(tipos_stack[tipos_top], "booleano");
                } else {
                    erro();
                    return NULL;
                }

                printf("\nDepois de op lógico - Topo atual: %d", tipos_top);
                printf("\nDepois de op lógico - Tipo no topo: %s", tipos_stack[tipos_top]);
            }
            else if (strstr(atual.lexema, "u")) {
                printf("\nAntes do Unário - Topo atual: %d", tipos_top);
                printf("\nAntes do Unário - Tipo no topo: %s", tipos_stack[tipos_top]);

                if (tipos_top < 0 || strcmp(tipos_stack[tipos_top], "inteiro") != 0) {
                    erro();
                    return NULL;
                }
                strcpy(tipos_stack[tipos_top], "inteiro");

                printf("\nDepois do Unário - Topo atual: %d", tipos_top);
                printf("\nDepois do Unário - Tipo no topo: %s", tipos_stack[tipos_top]);
            }
            else {
                printf("\nAntes da operação binária - Topo atual: %d", tipos_top);
                printf("\nAntes da operação binária - Tipo no topo: %s", tipos_stack[tipos_top]);

                if (tipos_top < 1) {
                    erro();
                    return NULL;
                }
                const char* tipo2 = tipos_stack[tipos_top--];
                const char* tipo1 = tipos_stack[tipos_top];

                if (atual.simbolo == TOKEN_SDIV) {
                    if (strcmp(tipo1, "inteiro") != 0 || strcmp(tipo2, "inteiro") != 0) {
                        erro();
                        return NULL;
                    }
                    strcpy(tipos_stack[tipos_top], "inteiro");
                }
                else if (atual.simbolo == TOKEN_SMAIS || atual.simbolo == TOKEN_SMENOS ||
                         atual.simbolo == TOKEN_SMULT) {
                    if (strcmp(tipo1, "inteiro") != 0 || strcmp(tipo2, "inteiro") != 0) {
                        erro();
                        return NULL;
                    }
                    strcpy(tipos_stack[tipos_top], "inteiro");
                }
                else if (atual.simbolo == TOKEN_SMAIOR || atual.simbolo == TOKEN_SMENOR ||
                         atual.simbolo == TOKEN_SIGUAL || atual.simbolo == TOKEN_SDIFERENTE ||
                         atual.simbolo == TOKEN_SMAIOR_IGUAL || atual.simbolo == TOKEN_SMENOR_IGUAL) {
                    if (strcmp(tipo1, "inteiro") != 0 || strcmp(tipo2, "inteiro") != 0) {
                        erro();
                        return NULL;
                    }
                    strcpy(tipos_stack[tipos_top], "booleano");
                }

                printf("\nDepois da operação binária - Topo atual: %d", tipos_top);
                printf("\nDepois da operação binária - Tipo no topo: %s", tipos_stack[tipos_top]);
            }
        }
    }

    printf("\nFinal - Topo atual: %d", tipos_top);
    printf("\nFinal - Tipo no topo: %s", tipos_stack[tipos_top]);

    return (tipos_top == 0) ? tipos_stack[0] : NULL;
}


void gera_codigo_posfixa(Expressao* posfixa) {
    for(int i = 0; i < posfixa->tamanho; i++) {
        Token token = posfixa->tokens[i];

        if(token.simbolo == TOKEN_DIGITO) {
            GERA("", "LDC", token.lexema, "");
        }
        else if(token.simbolo == TOKEN_SVERDADEIRO) {
            GERA("", "LDC", "1", "");  // Adicionado
        }
        else if(token.simbolo == TOKEN_SFALSO) {
            GERA("", "LDC", "0", "");  // Adicionado
        }

        else if(token.simbolo == TOKEN_IDENTIFICADOR) {
            Node* var = pesquisa_tabela(tabelaSimbolos, token.lexema, nivel);
            if(var != NULL) {
                if(strcmp(var->tipo, "booleano") == 0) {
                    char* endereco = int_to_str(var->memoria);
                    GERA("", "LDV", endereco, "");
                    free(endereco);
                }
                else if(strcmp(var->tipo, "funcao inteiro") == 0 ||
                        strcmp(var->tipo, "funcao booleana") == 0) {
                    char* endereco = int_to_str(0);
                    GERA("", "LDV", endereco, "");
                    free(endereco);
                }
                else {  // inteiro
                    char* endereco = int_to_str(var->memoria);
                    GERA("", "LDV", endereco, "");
                    free(endereco);
                }
            }
        }
        else {
            // Operadores
            switch(token.simbolo) {
                // Operadores Relacionais
                case TOKEN_SMENOR:
                    GERA("", "CME", "", "");
                    break;
                case TOKEN_SMAIOR:
                    GERA("", "CMA", "", "");
                    break;
                case TOKEN_SIGUAL:
                    GERA("", "CEQ", "", "");
                    break;
                case TOKEN_SDIFERENTE:
                    GERA("", "CDIF", "", "");
                    break;
                case TOKEN_SMAIOR_IGUAL:
                    GERA("", "CMAQ", "", "");
                    break;
                case TOKEN_SMENOR_IGUAL:
                    GERA("", "CMEQ", "", "");
                    break;

                // Operadores Aritméticos
                case TOKEN_SMAIS:
                    // Verifica se é unário (pelo lexema modificado em converter_para_posfixa)
                    if(strstr(token.lexema, "u") != NULL) {
                        // Operador unário + não precisa gerar código
                    } else {
                        GERA("", "ADD", "", "");
                    }
                    break;
                case TOKEN_SMENOS:
                    // Verifica se é unário (pelo lexema modificado em converter_para_posfixa)
                    if(strstr(token.lexema, "u") != NULL) {
                        GERA("", "INV", "", "");
                    } else {
                        GERA("", "SUB", "", "");
                    }
                    break;
                case TOKEN_SMULT:
                    GERA("", "MULT", "", "");
                    break;
                case TOKEN_SDIV:
                    GERA("", "DIVI", "", "");
                    break;

                // Operadores Lógicos
                case TOKEN_SE:
                    GERA("", "AND", "", "");
                    break;
                case TOKEN_SOU:
                    GERA("", "OR", "", "");
                    break;
                case TOKEN_SNAO:
                    GERA("", "NEG", "", "");
                    break;

                // Caso não seja nenhum dos operadores conhecidos
                default:
                    printf("\nOperador não reconhecido na geração de código\n");
                    break;
            }
        }
    }
}

//Compilador

void cleanup_tabela_simbolos() {
    while (tabelaSimbolos != NULL) {
        Node* temp = tabelaSimbolos;
        tabelaSimbolos = tabelaSimbolos->next;
        free(temp);
    }
}

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
            if(strcmp(current->tipo, "inteiro") == 0)
            return current;
       }
       current = current->next;
   }
   return NULL;
}

// Nova função específica para busca de tipos
Node* pesquisa_tipo_tabela(Node* top, const char* lexema, int nivel) {
    Node* current = top;
    Node* global_match = NULL;

    while (current != NULL) {
        if (strcmp(current->lexema, lexema) == 0) {
            if (current->escopo == nivel) {
                return current;
            }
            else if (current->escopo == 0 && global_match == NULL) {
                global_match = current;
            }
        }
        current = current->next;
    }

    return global_match;
}

Node* pesquisa_tabela(Node* top, const char* lexema, int nivel) {
    Node* current = top;
    Node* global_match = NULL;  // para guardar match do escopo global

    printf("Analisador Léxico encontrou: Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);

    printf("\nProcurando variável %s no nível %d", lexema, nivel);
    while (current != NULL) {
        printf("\nAnalisando: lexema=%s, escopo=%d, tipo=%s, memoria=%d",
               current->lexema, current->escopo, current->tipo, current->memoria);
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

void Desempilha(Node** top, int nivel) {
   printf("\n=== Iniciando Desempilha ===\n");
   printf("Nivel atual: %d\n", nivel);

   Node* current = *top;
   Node* proc_node = NULL;
   bool encontrou_marca = false;

   printf("\nEstado atual da tabela antes de qualquer alteração:\n");
   imprimirTabela(*top);

   // Primeiro encontra o procedimento marcado
   printf("\nProcurando procedimento marcado...\n");
   while (current != NULL) {
       printf("Analisando nó: lexema=%s, escopo=%d, tipo=%s, memoria=%d\n",
              current->lexema, current->escopo, current->tipo, current->memoria);

       if (current->escopo == 1) {
           encontrou_marca = true;
           proc_node = current;
           printf(">>> Encontrou procedimento marcado: %s\n", current->lexema);
           break;
       }
       current = current->next;
   }

   if (encontrou_marca && proc_node != NULL) {
       // Remove os nós até o procedimento
       printf("\nIniciando remoção dos nós...\n");

       while (*top != proc_node && *top != NULL) {
           Node* temp = *top;
           printf("Removendo nó: %s\n", temp->lexema);
           *top = (*top)->next;
           free(temp);
       }

       printf("Removendo marca do procedimento %s\n", proc_node->lexema);
       proc_node->escopo = 0;  // Remove a marca do procedimento

       printf("\nEstado final da tabela:\n");
       imprimirTabela(*top);
   } else {
       printf("\nNão encontrou procedimento marcado. Nenhuma ação realizada.\n");
   }

   printf("\n=== Fim do Desempilha ===\n");
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

void Analisa_chamada_funcao() {
    Node* var = pesquisa_declfunc_tabela(tabelaSimbolos, token.lexema);
    if(var != NULL) {
        char rotulo[10];
        sprintf(rotulo, "%d", var->memoria);
        GERA("", "CALL", rotulo, "");
        token = LerProximoToken(arquivo, &caractere);
    } else {
        printf("\n\nFunção não encontrada na tabela de símbolos\n\n");
        erro();
        return;
    }
}

void Analisa_chamada_de_procedimento(char novo[50]) {
    Node *var = pesquisa_declproc_tabela(tabelaSimbolos, novo);
    if(var == NULL) {
        printf("\n\nErro! Esse procedimento não está na tabela de símbolos\n\n");
        erro();
        return;
    } else {
        // Gera chamada usando o rótulo numérico do procedimento
        char rotulo[10];
        sprintf(rotulo, "%d", var->memoria);  // Usa o campo memoria para guardar número do rótulo
        GERA("", "CALL", rotulo, ""); // ta fazendo

        /*if(token.simbolo != TOKEN_SPONTO_VIRGULA) {
            printf("\n\nErro! Era esperado um ponto e virgula\n\n");
            erro();
            return;
        }*/
    }
}

void Analisa_fator() {
    printf("\n\nEntrou no analisa fator \n\n");
    if(token.simbolo == TOKEN_IDENTIFICADOR) {
        Node* resultado = pesquisa_tabela(tabelaSimbolos, token.lexema, nivel);
        adicionar_token(expressao_atual, token);

        if(resultado != NULL) {
            if(strcmp(resultado->tipo, "funcao inteiro") == 0 ||
               strcmp(resultado->tipo, "funcao booleana") == 0) {
                Analisa_chamada_funcao();
            } else {
                token = LerProximoToken(arquivo, &caractere);
            }
        } else {
            printf("\nErro: Identificador não encontrado na tabela de símbolos\n");
            erro();
            return;
        }
    } else if(token.simbolo == TOKEN_DIGITO) {
        adicionar_token(expressao_atual, token);
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        token = LerProximoToken(arquivo, &caractere);
    } else if(token.simbolo == TOKEN_SNAO) {
        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);
        Analisa_fator();
    } else if(token.simbolo == TOKEN_SABRE_PARENTESES) {
        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);
        Analisa_expressao();
        if(token.simbolo == TOKEN_SFECHA_PARENTESES) {
            adicionar_token(expressao_atual, token);
            token = LerProximoToken(arquivo, &caractere);
            printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
        } else {
            printf("\n\nEra esperado um fecha parenteses\n\n");
            erro();
            return;
        }
    } else if(strcmp(token.lexema, "verdadeiro") == 0 || strcmp(token.lexema, "falso") == 0) {
        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);
    } else {
        printf("\n\nEra esperado um fator válido\n\n");
        erro();
        return;
    }
}

void Analisa_termo(){
    Analisa_fator();
    while(token.simbolo == TOKEN_SMULT || token.simbolo == TOKEN_SDIV || token.simbolo == TOKEN_SE){
        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);
        Analisa_fator();
    }
}

void Analisa_expressao_simples(){
    if(token.simbolo == TOKEN_SMAIS || token.simbolo == TOKEN_SMENOS){
        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);
    }
    Analisa_termo();
    while(token.simbolo == TOKEN_SMAIS || token.simbolo == TOKEN_SMENOS || token.simbolo == TOKEN_SOU){
        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);
        Analisa_termo();
    }
}

void Analisa_expressao() {
    if (expressao_atual == NULL) {
        expressao_atual = criar_expressao(100);
        if (expressao_atual == NULL) {
            erro();
            return;
        }
    }
    Analisa_expressao_simples();


    if(token.simbolo == TOKEN_SMAIOR || token.simbolo == TOKEN_SMENOR ||
       token.simbolo == TOKEN_SIGUAL || token.simbolo == TOKEN_SDIFERENTE ||
       token.simbolo == TOKEN_SMAIOR_IGUAL || token.simbolo == TOKEN_SMENOR_IGUAL) {


        adicionar_token(expressao_atual, token);
        token = LerProximoToken(arquivo, &caractere);


        Analisa_expressao_simples();
    }
}

void Analisa_declaracao_funcao() {
  char auxiliar[100];
    token = LerProximoToken(arquivo, &caractere);
    nivel = 1;

    if (token.simbolo == TOKEN_IDENTIFICADOR) {
        Node* var = pesquisa_declfunc_tabela(tabelaSimbolos, token.lexema);
        if (var == NULL) {
            int numero_rotulo = rotulo_geracao;
            strcpy(auxiliar, token.lexema);
            inserirNaTabela(&tabelaSimbolos, token.lexema, nivel, "funcao", rotulo_geracao);
            rotulo_geracao++;

            token = LerProximoToken(arquivo, &caractere);
            if (token.simbolo == TOKEN_SDOISPONTOS) {
                token = LerProximoToken(arquivo, &caractere);
                if (token.simbolo == TOKEN_SINTEIRO || token.simbolo == TOKEN_SBOOLEANO) {
                    if(token.simbolo == TOKEN_SINTEIRO) {
                        TABSIMB(tabelaSimbolos, auxiliar, 0);
                    }
                    else {
                        TABSIMB(tabelaSimbolos, auxiliar, 1);
                    }

                    token = LerProximoToken(arquivo, &caractere);
                    if (token.simbolo == TOKEN_SPONTO_VIRGULA) {
                        char rot[10];
                        sprintf(rot, "%d", numero_rotulo);
                        GERA(rot, "NULL", "", "");  // Rótulo da função

                        Analisa_bloco();

                       // Gera código para desalocação e retorno
                        Node* proc_node = NULL;
                        Node* current = tabelaSimbolos;
                        int num_vars = 0;
                        int pos_memoria = -1;

                        // Primeiro encontra o procedimento marcado
                        while (current != NULL) {
                            if (current->escopo == 1) {
                                proc_node = current;
                                break;
                            }
                            current = current->next;
                        }

                        // Se encontrou o procedimento, conta as variáveis até ele
                        if (proc_node) {
                            current = tabelaSimbolos;
                            while (current != proc_node) {
                                if (strcmp(current->tipo, "inteiro") == 0 ||
                                    strcmp(current->tipo, "booleano") == 0) {
                                    num_vars++;
                                    if (pos_memoria == -1 || current->memoria < pos_memoria) {
                                        pos_memoria = current->memoria;
                                    }
                                }
                                current = current->next;
                            }
                        }

                        if (num_vars > 0) {
                            char* p3 = int_to_str(pos_memoria);
                            char* p4 = int_to_str(num_vars);
                            GERA("", "DALLOC", p3, p4);
                            add_free_space(atoi(p3), atoi(p4));
                            free(p3);
                            free(p4);
                        }
                        GERA("", "RETURN", "", "");
                  } else {
                       printf("\n\nEra esperado um Ponto e Vírgula\n\n");
                       erro();
                       return;
                  }
              } else {
                  printf("\n\nEra esperado um Inteiro ou um Booleano\n\n");
                  erro();
                  return;
              }
          } else {
              printf("\n\nEra esperado um Dois Pontos\n\n");
              erro();
              return;
          }
      } else {
          printf("\n\nJá existe um elemento na tabela com o nome desta função\n\n");
          erro();
          return;
      }
  } else {
      printf("\n\nEra esperado um identificador\n\n");
      erro();
      return;
  }
  Desempilha(&tabelaSimbolos, nivel);
}

// Função Analisa_declaracao_procedimento corrigida
void Analisa_declaracao_procedimento() {
    token = LerProximoToken(arquivo, &caractere);
    nivel = 1;  // marca novo galho

    if(token.simbolo == TOKEN_IDENTIFICADOR) {
        Node* var = pesquisa_declproc_tabela(tabelaSimbolos, token.lexema);
        if(var == NULL) {
            // Guarda rótulo atual
            int numero_rotulo = rotulo_geracao;

            // Insere na tabela com o rótulo
            inserirNaTabela(&tabelaSimbolos, token.lexema, nivel, "procedimento", rotulo_geracao);
            rotulo_geracao++;

            token = LerProximoToken(arquivo, &caractere);
            if(token.simbolo == TOKEN_SPONTO_VIRGULA) {
                // Gera rótulo no início do procedimento
                char rot[10];
                sprintf(rot, "%d", numero_rotulo);
                GERA(rot, "NULL", "", "");

                Analisa_bloco();

                // Gera código para desalocação e retorno
                Node* proc_node = NULL;
                Node* current = tabelaSimbolos;
                int num_vars = 0;
                int pos_memoria = -1;

                // Primeiro encontra o procedimento marcado
                while (current != NULL) {
                    if (current->escopo == 1) {
                        proc_node = current;
                        break;
                    }
                    current = current->next;
                }

                // Se encontrou o procedimento, conta as variáveis até ele
                if (proc_node) {
                    current = tabelaSimbolos;
                    while (current != proc_node) {
                        if (strcmp(current->tipo, "inteiro") == 0 ||
                            strcmp(current->tipo, "booleano") == 0) {
                            num_vars++;
                            if (pos_memoria == -1 || current->memoria < pos_memoria) {
                                pos_memoria = current->memoria;
                            }
                        }
                        current = current->next;
                    }
                }

                if (num_vars > 0) {
                    char* p3 = int_to_str(pos_memoria);
                    char* p4 = int_to_str(num_vars);
                    GERA("", "DALLOC", p3, p4);
                    add_free_space(atoi(p3), atoi(p4));
                    free(p3);
                    free(p4);
                }
                GERA("", "RETURN", "", "");
            } else {
                erro();
                return;
            }
        } else {
            printf("\n\nERRO: Já está na tabela\n\n");
            erro();
            return;
        }
    } else {
        erro();
        return;
    }

    Desempilha(&tabelaSimbolos, nivel);
}

void Analisa_se() {
    printf("\nIniciando análise do comando se");

    token = LerProximoToken(arquivo, &caractere);
    printf("\nToken após 'se': Lexema: %s, Token: %s", token.lexema, Nomedotoken[token.simbolo]);

    expressao_atual = criar_expressao(100);
    if (!expressao_atual) {
        printf("\nErro ao alocar memória para expressão");
        erro();
        return;
    }


    Analisa_expressao();

    if (expressao_atual == NULL || expressao_atual->tamanho == 0) {
        printf("\nErro: Expressão vazia detectada");
        erro();
        return;
    }

    Expressao* posfixa = converter_para_posfixa(expressao_atual);
    if (posfixa) {
        printf("\nExpressão convertida para pós-fixa:");
        imprimir_expressao(posfixa, "");

        const char* tipo_resultado = analisar_tipos_posfixa(posfixa);
        if(tipo_resultado!=NULL){
        if (strcmp(tipo_resultado, "booleano") == 0) {
            if (!tipo_resultado) {
                printf("\nErro: Tipos incompatíveis na expressão\n");
                erro();
                return;
            }

            gera_codigo_posfixa(posfixa);
            printf("\nGerou código da condição");

            char rotulo[10];
            sprintf(rotulo, "%d", rotulo_geracao);
            GERA("", "JMPF", rotulo, "");
            printf("\nGerou desvio condicional (JMPF) para %s", rotulo);
            rotulo_geracao++;

            if (token.simbolo == TOKEN_SENTAO) {
                printf("\nEncontrou token 'entao'");
                token = LerProximoToken(arquivo, &caractere);
                Analisa_comando_simples();

                if (token.simbolo == TOKEN_SSENAO) {
                    printf("\nEncontrou token 'senao'");

                    char r_fim[10];
                    sprintf(r_fim, "%d", rotulo_geracao);
                    GERA("", "JMP", r_fim, "");
                    printf("\nGerou desvio incondicional (JMP) para %s", r_fim);
                    rotulo_geracao++;

                    GERA(rotulo, "NULL", "", "");
                    printf("\nGerou label do else: %s", rotulo);

                    token = LerProximoToken(arquivo, &caractere);
                    Analisa_comando_simples();

                    GERA(r_fim, "NULL", "", "");
                    printf("\nGerou label do fim: %s", r_fim);
                } else {
                    GERA(rotulo, "NULL", "", "");
                    printf("\nGerou label do else (sem senao): %s", rotulo);
                }
            } else {
                printf("\n\nEra esperado um 'entao'\n\n");

                if (posfixa->tokens) {
                    free(posfixa->tokens);
                }
                free(posfixa);

                if (expressao_atual->tokens) {
                    free(expressao_atual->tokens);
                }
                free(expressao_atual);
                expressao_atual = NULL;

                erro();
                return;
            }
        } else {
            printf("\n\nERRO! Não é Booleano\n\n");
            erro();
            return;
        }
        }else{
            printf("\nErro! Operação Inválida com Tipos\n");
            erro();
            return;
        }

        if (posfixa->tokens) {
            free(posfixa->tokens);
        }
        free(posfixa);
    }

    printf("\n\nExpressao atual: %c", expressao_atual);

    if (expressao_atual && expressao_atual->tokens) {
        free(expressao_atual->tokens);
    }
    if (expressao_atual) {
        free(expressao_atual);
        expressao_atual = NULL;
    }

    printf("\nFinalizou análise do comando se\n");
}


// Função Analisa_enquanto corrigida
void Analisa_enquanto() {
    int auxrot1, auxrot2;
    auxrot1 = rotulo_geracao++;
    auxrot2 = rotulo_geracao++;
    char rot1[10];
    sprintf(rot1, "%d", auxrot1);
    GERA(rot1, "NULL", "", "");
    token = LerProximoToken(arquivo, &caractere);
    expressao_atual = criar_expressao(100);

    if (!expressao_atual) {
        printf("\nErro ao alocar memória para expressão");
        erro();
        return;
    }

    Analisa_expressao();

    if (expressao_atual == NULL || expressao_atual->tamanho == 0) {
        printf("\nErro: Expressão vazia detectada");
        erro();
        return;
    }

    Expressao* posfixa = converter_para_posfixa(expressao_atual);
    if (posfixa) {
        printf("\nExpressão convertida para pós-fixa:");
        imprimir_expressao(posfixa, "");

        const char* tipo_resultado = analisar_tipos_posfixa(posfixa);
        if (tipo_resultado != NULL) {
            if (strcmp(tipo_resultado, "booleano") == 0) {
                if (!tipo_resultado) {
                    printf("\nErro: Tipos incompatíveis na expressão\n");
                    erro();
                    return;
                }
                gera_codigo_posfixa(posfixa);
                char rot2[10];
                sprintf(rot2, "%d", auxrot2);
                GERA("", "JMPF", rot2, "");

                if(token.simbolo == TOKEN_SFACA) {
                    token = LerProximoToken(arquivo, &caractere);
                    Analisa_comando_simples();
                    GERA("", "JMP", rot1, "");
                    GERA(rot2, "NULL", "", "");
                }
            } else {
                printf("\nErro: Expressão deve ser booleana\n");
                erro();
                return;
            }
        } else {
            printf("\nErro: Operação inválida com tipos\n");
            erro();
            return;
        }
        free(posfixa->tokens);
        free(posfixa);
    }

    if (expressao_atual) {
        free(expressao_atual->tokens);
        free(expressao_atual);
        expressao_atual = NULL;
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

                char* endereco = int_to_str(var->memoria);
                GERA("", "LDV", endereco, "");
                GERA("", "PRN", "", "");
                free(endereco);

                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo == TOKEN_SFECHA_PARENTESES){
                    token = LerProximoToken(arquivo, &caractere);
                }
                else{
                        printf("\n\nEra esperado um fecha parenteses\n\n");
                    erro();
                    return;
                }
            }
            else{
                printf("\n\nO identificador não está presente na tabela ou é do tipo errado\n\n");
                erro();
                return;
            }
        }
        else{
            printf("\n\nEra esperado um identificador\n\n");
            erro();
            return;
        }
    }
    else{
        printf("\n\nEra esperado um abre parenteses\n\n");
        erro();
        return;
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

                GERA("", "RD", "", "");
                char* endereco = int_to_str(var->memoria);
                GERA("", "STR", endereco, "");
                free(endereco);

                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo == TOKEN_SFECHA_PARENTESES){
                        printf("\n\nFechou o parenteses do analisa leia\n\n");
                    token = LerProximoToken(arquivo, &caractere);
                }
                else{
                        printf("\n\nEra esperado um fecha parenteses\n\n");
                    erro();
                    return;
                }
            }
            else{
                printf("\n\nO identificador não está presente na tabela ou  é do tipo errado\n\n");
                erro();
                return;
            }
        }
        else{
                printf("\n\nEra esperado um identificador\n\n");
            erro();
            return;
        }
    }
    else{
            printf("\n\nEra esperado um abre parenteses\n\n");
        erro();
        return;
    }
}

void Analisa_atribuicao(char* novo) {
    printf("\n=== Iniciando análise de atribuição para '%s' ===\n", novo);

    Node *var = pesquisa_declfunc_tabela(tabelaSimbolos, novo);
    if (!var) {
        printf("\nErro: Variável não encontrada");
        erro();
        return;
    }

    // Se for função, permite atribuir valor do tipo retornado
    const char* tipo_esperado = var->tipo;
    if (strstr(tipo_esperado, "funcao") != NULL) {
        if (strstr(tipo_esperado, "inteiro")) {
            tipo_esperado = "inteiro";
        } else if (strstr(tipo_esperado, "booleana")) {
            tipo_esperado = "booleano";
        }
    }

    printf("\nTipo da variável '%s': %s", novo, var->tipo);

    token = LerProximoToken(arquivo, &caractere);

    /*if (token.simbolo == TOKEN_SVERDADEIRO || token.simbolo == TOKEN_SFALSO) {
       GERA("", "LDC", token.simbolo == TOKEN_SVERDADEIRO ? "1" : "0", "");
       GERA("", "STR", int_to_str(var->memoria), "");
       token = LerProximoToken(arquivo, &caractere);
       return;
   }*/

    expressao_atual = criar_expressao(100);
    Analisa_expressao();

    printf("\n\nVAI COMEÇAR A CONVERSÃO PARA A PÓS FIXA\n\n");
    Expressao* posfixa = converter_para_posfixa(expressao_atual);
    if (posfixa) {
        printf("\nExpressão convertida para pós-fixa:");
        imprimir_expressao(posfixa, "");
        printf("\nTipo resultado: %s", tipo_resultado);
        printf("\nTipo esperado: %s", tipo_esperado);
        const char* tipo_resultado = analisar_tipos_posfixa(posfixa);
        if (!tipo_resultado || strcmp(tipo_resultado, tipo_esperado) != 0) {
            printf("\nErro: Tipos incompatíveis na atribuição\n");
            erro();
            return;
        }

        printf("\nExpressão convertida para pós-fixa:");
        imprimir_expressao(posfixa, "");

        printf("\nAnalisando tipos da expressão...");
        const char* tipo_exp = analisar_tipos_posfixa(posfixa);
        if (tipo_exp) {
            printf("\nTipo resultante da expressão: %s", tipo_exp);

            // Verifica compatibilidade
            if (strcmp(var->tipo, "booleano") == 0) {
                if (strcmp(tipo_exp, "booleano") != 0) {
                    printf("\nErro: Tentando atribuir %s a booleano", tipo_exp);
                    erro();
                    return;
                }
            } else if (strcmp(var->tipo, "inteiro") == 0) {
                if (strcmp(tipo_exp, "inteiro") != 0) {
                    printf("\nErro: Tentando atribuir %s a inteiro", tipo_exp);
                    erro();
                    return;
                }
            }

            gera_codigo_posfixa(posfixa);

            if(strcmp(var->tipo, "inteiro") == 0 || strcmp(var->tipo, "booleano") == 0){
                char* endereco = int_to_str(var->memoria);
                GERA("", "STR", endereco, "");
                free(endereco);
            }
            else{
                char* endereco = int_to_str(0);
                GERA("", "STR", endereco, "");
                free(endereco);
            }
        }

        free(posfixa->tokens);
        free(posfixa);
    }

    if (expressao_atual) {
        free(expressao_atual->tokens);
        free(expressao_atual);
        expressao_atual = NULL;
    }

    printf("\nToken após análise: %s", token.lexema);
    printf("\n=== Fim da análise de atribuição ===\n");
}

void Analisa_atrib_chprocedimento(){
    char novo[50];
    strcpy(novo, token.lexema);

    token = LerProximoToken(arquivo, &caractere);
    if(token.simbolo == TOKEN_ATRIBUICAO){
        Analisa_atribuicao(novo);
        printf("\n\nSaiu do Analisa Atribuição\n\n");
    }
    else{
        Analisa_chamada_de_procedimento(novo);
    }
}

void Analisa_comando_simples(){
    printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
    if(token.simbolo == TOKEN_IDENTIFICADOR){
        Analisa_atrib_chprocedimento();
        printf("\n\nSaiu do Analisa Atrib Ch Procedimento\n\n");
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

void Analisa_comandos() {
    if(token.simbolo == TOKEN_SINICIO) {
        token = LerProximoToken(arquivo, &caractere);
        Analisa_comando_simples();
        printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);

        while(token.simbolo != TOKEN_SFIM) {
            if(token.simbolo == TOKEN_SPONTO_VIRGULA) {
                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo != TOKEN_SFIM) {
                    Analisa_comando_simples();
                    printf("Lexema: %s, Token: %s\n", token.lexema, Nomedotoken[token.simbolo]);
                }
            }
            else {
                printf("\n\nEra esperado um ponto e vírgula\n\n");
                erro();
                return;
            }
        }
        token = LerProximoToken(arquivo, &caractere);
    }
    else {
        printf("\n\nEra esperado um Token Início ou uma nova subrotina / função\n\n");
        erro();
        return;
    }
}

void Analisa_tipo(){
    if(token.simbolo != TOKEN_SINTEIRO && token.simbolo != TOKEN_SBOOLEANO){
            printf("\n\nEra esperado um inteiro ou um Booleano\n\n");
        erro();
        return;
    }
    else{
        colocarTipoNasVariaveis(&tabelaSimbolos, token.lexema);
        printf("\nEntrou no Analisa_Tipo");
        imprimirTabela(tabelaSimbolos);
    }
    token = LerProximoToken(arquivo, &caractere);
}

// Função Analisa_Subrotinas corrigida
void Analisa_subrotinas() {
    int flag = 0;
    int auxrot;

    if(token.simbolo == TOKEN_SPROCEDIMENTO || token.simbolo == TOKEN_SFUNCAO) {
        auxrot = rotulo_geracao++;
        char rot[10];
        sprintf(rot, "%d", auxrot);
        GERA("", "JMP", rot, "");
        flag = 1;
    }

    while(token.simbolo == TOKEN_SPROCEDIMENTO || token.simbolo == TOKEN_SFUNCAO) {
        if(token.simbolo == TOKEN_SPROCEDIMENTO) {
            Analisa_declaracao_procedimento();
        }
        else {
            Analisa_declaracao_funcao();
        }

        if(token.simbolo == TOKEN_SPONTO_VIRGULA) {
            token = LerProximoToken(arquivo, &caractere);
        }
        else {
            erro();
            return;
        }
    }

    if(flag == 1) {
        char rot[10];
        sprintf(rot, "%d", auxrot);
        GERA(rot, "NULL", "", "");
    }
}

void Analisa_variaveis() {
    int qtd_var = 0;
    int pos_inicial = find_free_space(2); // Busca espaço livre inicialmente

    do {
        if(token.simbolo == TOKEN_IDENTIFICADOR) {
            qtd_var++;
            if(!Pesquisa_duplicvar_tabela(tabelaSimbolos, token.lexema)) {
                inserirNaTabela(&tabelaSimbolos, token.lexema, 0, "variavel", pos_inicial + (qtd_var - 1));
                token = LerProximoToken(arquivo, &caractere);
                if(token.simbolo == TOKEN_SVIRGULA || token.simbolo == TOKEN_SDOISPONTOS) {
                    if(token.simbolo == TOKEN_SVIRGULA) {
                        token = LerProximoToken(arquivo, &caractere);
                        if(token.simbolo == TOKEN_SDOISPONTOS) {
                            erro();
                            return;
                        }
                    }
                } else {
                    erro();
                    break;
                }
            } else {
                erro();
                break;
            }
        } else {
            erro();
            break;
        }
    } while(token.simbolo != TOKEN_SDOISPONTOS);

    char* p3 = int_to_str(pos_inicial);
    char* p4 = int_to_str(qtd_var);
    GERA("", "ALLOC", p3, p4);
    free(p3);
    free(p4);

    if (pos_inicial + qtd_var > end) {
        end = pos_inicial + qtd_var;
    }

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
                printf("\n\nEra esperado um identificador na declaração de variáveis\n\n");
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
    end = 1;           // Começa do 1, 0 para retorno
    rotulo_geracao = 1; // Começa do 1, L1 para programa principal

    if(token.simbolo == TOKEN_SPROGRAMA) {
        token = LerProximoToken(arquivo, &caractere);
        if(token.simbolo == TOKEN_IDENTIFICADOR) {
            inserirNaTabela(&tabelaSimbolos, token.lexema, 0, "nomedeprograma", 1);
            token = LerProximoToken(arquivo, &caractere);
            if(token.simbolo == TOKEN_SPONTO_VIRGULA) {
                GERA("", "START", "", "");
                GERA("", "ALLOC", "0", "1");  // Aloca variável de retorno
                Analisa_bloco();

                if(token.simbolo == TOKEN_SPONTO) {
                    Node* current = tabelaSimbolos;
                    int global_vars = 0;

                    while (current != NULL) {
                        if (current->escopo == 0 &&
                           (strcmp(current->tipo, "inteiro") == 0 ||
                            strcmp(current->tipo, "booleano") == 0)) {
                            global_vars++;
                        }
                        current = current->next;
                    }

                    if (global_vars > 0) {
                        char* p3 = int_to_str(1);  // Sempre começa em 1
                        char* p4 = int_to_str(global_vars);
                        GERA("", "DALLOC", p3, p4);
                        add_free_space(1, global_vars);
                        free(p3);
                        free(p4);
                    }

                    GERA("", "DALLOC", "0", "1");  // Desaloca variável de retorno
                    GERA("", "HLT", "", "");

                    token = LerProximoToken(arquivo, &caractere);
                    if(token.simbolo == TOKEN_ERRO && strcmp(token.lexema, "EOF") == 0) {
                        debug_print("Compilação finalizada");
                        return;
                    }
                } else {
                    printf("Erro no ponto");
                    erro();
                    return;
                }
            } else {
                printf("Erro no ponto e virgula");
                erro();
                return;
            }
        } else {
            printf("Erro no identificador");
            erro();
            return;
        }
    } else {
        printf("Erro no token programa");
        erro();
        return;
    }
}

void erro() {
    char mensagem[256];
    erro_encontrado = true;

    #ifdef GUI_MODE
        int coluna = 1;
        if(token.simbolo != TOKEN_ERRO) {
            sprintf(mensagem, "%d:%d|Erro - Token atual: '%s' (%s)",
                    linhas, coluna, token.lexema, Nomedotoken[token.simbolo]);
        } else {
            sprintf(mensagem, "%d:%d|Erro - Token inválido",
                    linhas, coluna);
        }
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)mensagem);
        SetWindowText(hStatus, strchr(mensagem, '|') + 1);

        // Forçar finalização da compilação
        if (arquivo_obj != NULL) {
            fclose(arquivo_obj);
            arquivo_obj = NULL;
        }

        if (arquivo != NULL) {
            fclose(arquivo);
            arquivo = NULL;
        }

        // Limpa recursos pendentes
        if (expressao_atual) {
            if (expressao_atual->tokens) free(expressao_atual->tokens);
            free(expressao_atual);
            expressao_atual = NULL;
        }
    #else
        printf("\n\nErro!\n\n");
        printf("\n\nErro na linha: %d\n\n", linhas);
        exit(1);  // No modo console podemos usar exit
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
    setlocale(LC_ALL, "Portuguese_Brazil.1252");
    if (argc > 1) {  // Modo console
        linhas = 1;
        arquivo = fopen("teste.txt", "r");
        if (arquivo == NULL) {
            printf("Erro ao abrir o arquivo.\n");
            return 1;
        }
        caractere = LerCaracter(arquivo);
        token = LerProximoToken(arquivo, &caractere);
        if (token.simbolo != TOKEN_ERRO) {
            AnalisadorSintatico();
            printf("\n\nsaiu\n\n");
        } else {
            printf("Erro: Token inválido encontrado.\n");
        }
        fclose(arquivo);
        contador_linhas(arquivo, caractere, linhas);
        cleanup_tabela_simbolos();
        return 0;
    }
    else {  // Modo GUI
        #ifdef _WIN32
            return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
        #else
            printf("Modo GUI só está disponível no Windows!\n");
            return 1;
        #endif
    }
}

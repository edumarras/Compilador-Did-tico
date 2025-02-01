#define _WIN32_IE 0x0300
#define WINVER 0x0500
#define _WIN32_WINDOWS 0x0410
#define _WIN32_WINNT 0x0500
#define ARROW_COLUMN_WIDTH 30

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <locale.h>
#include <windowsx.h>

// Constantes MVD
#define MAX_STACK 1024
#define MAX_CODE 1024

// IDs dos controles
#define ID_ARQUIVO_ABRIR 101
#define ID_SOBRE        102
#define ID_CODE_LIST    103
#define ID_MEMORY_LIST  104
#define ID_OUTPUT       105
#define ID_COMBO_MODE   106
#define ID_BTN_STOP     107
#define ID_BTN_RUN      108

#define IDD_INPUT 201
#define IDC_INPUT 202

// Estrutura da MVD
typedef struct {
    int stack[MAX_STACK];  // Pilha M
    int stackPointer;      // Registrador s
    int programCounter;    // Registrador i
    int code[MAX_CODE];    // Região P
    int running;           // Estado de execução
    int stepMode;         // Modo passo-a-passo
} MVDState;

// Instruções MVD
enum Instructions {
    START, ALLOC, DALLOC, HLT,
    LDC, LDV, ADD, SUB, MULT, DIVI, INV,
    AND, OR, NEG, CME, CMA, CEQ, CDIF, CMEQ, CMAQ,
    JMP, JMPF, NULL_, STR,
    RD, PRN, CALL, RETURN
};

// Variáveis globais
const char className[] = "SimuladorClass";
MVDState mvd;
HWND hwndCodeList;    // Lista do código
HWND hwndMemoryList;  // Lista de memória/pilha
HWND hwndOutput;      // Área de saída
HWND hwndModeCombo;   // Combobox de modo
HWND hwndStopBtn;     // Botão parar
HWND hwndRunBtn;      // Botão executar
int labelMap[MAX_CODE] = {0};  // Mapa de rótulos

// Protótipos
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeUI(HWND hwnd);
void CreateCodeList(HWND hwnd);
void CreateMemoryList(HWND hwnd);
void CreateControls(HWND hwnd);
void ResizeWindow(HWND hwnd);
void executeInstruction(HWND hwnd, int* labelMap);
void updateStackView();
void updateCodeHighlight();
void onRun(HWND hwnd);
void initMVD();
void loadMVDFile(HWND hwnd);
int parseInstruction(const char* instr);
LRESULT CALLBACK InputDlgProc(HWND, UINT, WPARAM, LPARAM);

// Funções MVD

LRESULT CALLBACK InputDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int value;
    switch(msg) {
        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hwnd, IDC_INPUT));
            return TRUE;

        case WM_COMMAND:
            if(LOWORD(wParam) == IDOK) {
                char buffer[256];
                GetDlgItemText(hwnd, IDC_INPUT, buffer, 256);
                value = atoi(buffer);
                EndDialog(hwnd, value);
                return TRUE;
            }
            if(LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, 0);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void loadMVDFile(HWND hwnd) {
    char fileName[MAX_PATH];
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    char buffer[256];
    FILE *file;
    int lineNum = 1;

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Arquivos OBJ\0*.obj\0Todos\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    fileName[0] = '\0';

    if(GetOpenFileName(&ofn)) {
        file = fopen(fileName, "r");
        if(!file) return;

        ListView_DeleteAllItems(hwndCodeList);
        initMVD();

        // Primeira passagem: mapear rótulos para linhas
        while(fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0;
            buffer[strcspn(buffer, "\r")] = 0;

            char rotulo[5] = "";
            strncpy(rotulo, buffer, 4);
            rotulo[4] = '\0';
            while(strlen(rotulo) > 0 && rotulo[strlen(rotulo)-1] == ' ')
                rotulo[strlen(rotulo)-1] = '\0';

            if(strlen(rotulo) > 0 && isdigit(rotulo[0])) {
                int label = atoi(rotulo);
                labelMap[label] = lineNum;
            }
            lineNum++;
        }

        rewind(file);
        lineNum = 1;

        // Segunda passagem: carregar instruções e resolver JMPs
        while(fgets(buffer, sizeof(buffer), file)) {
            LVITEM lvi = {0};
            char lineText[32];
            char rotulo[5] = "";
            char instruction[9] = "";
            char attr1[5] = "";
            char attr2[5] = "";
            int codePos = lineNum - 1;

            buffer[strcspn(buffer, "\n")] = 0;
            buffer[strcspn(buffer, "\r")] = 0;

            strncpy(rotulo, buffer, 4);
            strncpy(instruction, buffer + 4, 8);
            strncpy(attr1, buffer + 12, 4);
            strncpy(attr2, buffer + 16, 4);

            rotulo[4] = instruction[8] = attr1[4] = attr2[4] = '\0';

            // Remover espaços em branco
            while(strlen(rotulo) > 0 && rotulo[strlen(rotulo)-1] == ' ')
                rotulo[strlen(rotulo)-1] = '\0';
            while(strlen(instruction) > 0 && instruction[strlen(instruction)-1] == ' ')
                instruction[strlen(instruction)-1] = '\0';
            while(strlen(attr1) > 0 && attr1[strlen(attr1)-1] == ' ')
                attr1[strlen(attr1)-1] = '\0';
            while(strlen(attr2) > 0 && attr2[strlen(attr2)-1] == ' ')
                attr2[strlen(attr2)-1] = '\0';

            // Inserir na lista visual
            lvi.mask = LVIF_TEXT;
            lvi.iItem = codePos;

            sprintf(lineText, "%d", lineNum);
            lvi.iSubItem = 0;
            lvi.pszText = lineText;
            ListView_InsertItem(hwndCodeList, &lvi);

            if(strlen(instruction) > 0) {
                int instCode = parseInstruction(instruction);
                int currentPos = (lineNum - 1) * 3;
                mvd.code[currentPos] = instCode;

                // Se for JMP ou JMPF, ajustar o atributo para o número da linha
                if(instCode == JMP || instCode == JMPF) {
                    int targetLabel = atoi(attr1);
                    int targetLine = labelMap[targetLabel];
                    sprintf(attr1, "%d", targetLine); // Atualiza attr1 com o número da linha
                }

                if(instCode == CALL) {
                    int targetLabel = atoi(attr1);
                    int targetLine = labelMap[targetLabel];
                    sprintf(attr1, "%d", targetLine);
                }

                lvi.iSubItem = 1;
                lvi.pszText = instruction;
                ListView_SetItem(hwndCodeList, &lvi);

                if(strlen(attr1) > 0) {
                    lvi.iSubItem = 2;
                    lvi.pszText = attr1;
                    ListView_SetItem(hwndCodeList, &lvi);

                    mvd.code[currentPos + 1] = atoi(attr1);

                    if(strlen(attr2) > 0) {
                        lvi.iSubItem = 3;
                        lvi.pszText = attr2;
                        ListView_SetItem(hwndCodeList, &lvi);

                        mvd.code[currentPos + 2] = atoi(attr2);
                    }
                }
            }
            lineNum++;
        }

        fclose(file);
        mvd.running = 1;
    }
}

void executeInstruction(HWND hwnd, int* labelMap) {
    int inst = mvd.code[mvd.programCounter];
    int temp;
    printf("\nExecutando instrução na posição %d: código %d\n", mvd.programCounter/3, inst);
    printf("Stack pointer antes: %d\n", mvd.stackPointer);

    switch(inst) {
        case START: {
            printf("Executando START\n");
            mvd.stackPointer = -1;
            mvd.programCounter += 3;
            break;
        }

        case ALLOC: {
           int m = mvd.code[mvd.programCounter + 1];
           int n = mvd.code[mvd.programCounter + 2];

           for(int k = 0; k < n; k++) {
               mvd.stackPointer++;
               mvd.stack[mvd.stackPointer] = mvd.stack[m + k];
           }

           mvd.programCounter += 3;
           break;
        }

        case DALLOC: {
            int m = mvd.code[mvd.programCounter + 1];
            int n = mvd.code[mvd.programCounter + 2];
            printf("Executando DALLOC m=%d n=%d\n", m, n);

            // Cada variável é copiada individualmente do topo
            for(int k = n-1; k >= 0; k--) {
                mvd.stack[m + k] = mvd.stack[mvd.stackPointer];
                mvd.stackPointer--;  // Reduz stackPointer para cada variável
            }
            mvd.programCounter += 3;
            break;
        }

        case LDC: {
            printf("Executando LDC\n");
            mvd.stackPointer++;
            mvd.stack[mvd.stackPointer] = mvd.code[mvd.programCounter + 1];
            mvd.programCounter += 3;
            break;
        }

        case LDV: {
            printf("Executando LDV\n");
            mvd.stackPointer++;
            mvd.stack[mvd.stackPointer] = mvd.stack[mvd.code[mvd.programCounter + 1]];
            mvd.programCounter += 3;
            break;
        }

        case ADD: {
            printf("ADD: %d + %d = %d\n",
                   mvd.stack[mvd.stackPointer-1],
                   mvd.stack[mvd.stackPointer],
                   mvd.stack[mvd.stackPointer-1] + mvd.stack[mvd.stackPointer]);
            mvd.stack[mvd.stackPointer-1] += mvd.stack[mvd.stackPointer];
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case SUB: {
            mvd.stack[mvd.stackPointer-1] -= mvd.stack[mvd.stackPointer];
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case MULT: {
            mvd.stack[mvd.stackPointer-1] *= mvd.stack[mvd.stackPointer];
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case DIVI: {
            mvd.stack[mvd.stackPointer-1] /= mvd.stack[mvd.stackPointer];
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case JMP: {
            int targetLine = mvd.code[mvd.programCounter + 1];
            mvd.programCounter = (targetLine - 1) * 3;
            printf("Executando JMP\n");
            break;
        }

        case NEG: {
            printf("Executando NEG\n");
            mvd.stack[mvd.stackPointer] = mvd.stack[mvd.stackPointer] == 0 ? 1 : 0;
            mvd.programCounter += 3;
            break;
        }

        case JMPF: {
                printf("Executando JMPF\n");
                if(mvd.stack[mvd.stackPointer] == 0) {
                    int targetLine = mvd.code[mvd.programCounter + 1];
                    mvd.programCounter = (targetLine - 1) * 3;
                } else {
                    mvd.programCounter += 3;
                }
                mvd.stackPointer--;
                break;
            }

        case NULL_: {
            printf("Executando NULL\n");
            mvd.programCounter += 3;  // Avança 3 posições
            break;
        }

        case STR: {
            printf("Executando STR\n");
            mvd.stack[mvd.code[mvd.programCounter + 1]] = mvd.stack[mvd.stackPointer];
            mvd.stackPointer--;
            mvd.programCounter += 3;  // Avança 3 posições
            break;
        }

        case RD: {
            printf("Executando RD\n");
            char buffer[256];
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = 200;
            int height = 25;
            int x = rect.right - 280;
            int y = rect.bottom - 60;

            HWND input = CreateWindow("EDIT", "",
                WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                x, y, width, height,
                hwnd, NULL, GetModuleHandle(NULL), NULL);
            ShowWindow(input, SW_SHOW);
            SetFocus(input);

            BOOL validInput = FALSE;
            MSG msg;

            while (GetMessage(&msg, NULL, 0, 0)) {
                if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
                    GetWindowText(input, buffer, 256);
                    if(strlen(buffer) > 0) {
                        validInput = TRUE;
                        mvd.stackPointer++;
                        mvd.stack[mvd.stackPointer] = atoi(buffer);
                        DestroyWindow(input);
                        mvd.programCounter += 3;
                        break;
                    }
                    // If empty input, just reset focus
                    SetFocus(input);
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!validInput) {
                DestroyWindow(input);
            }
            break;
        }

        case PRN: {
            char output[256];
            sprintf(output, "%d\n", mvd.stack[mvd.stackPointer]);
            int len = GetWindowTextLength(hwndOutput);
            SendMessage(hwndOutput, EM_SETSEL, len, len);
            SendMessage(hwndOutput, EM_REPLACESEL, FALSE, (LPARAM)output);

            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case CALL: {
            int targetLine = mvd.code[mvd.programCounter + 1];
            mvd.stackPointer++;
            mvd.stack[mvd.stackPointer] = (mvd.programCounter / 3) + 2;
            printf("CALL: Linha atual: %d, Próxima linha: %d, Saltando para: %d\n",
                   mvd.programCounter/3 + 1,
                   (mvd.programCounter/3) + 2,
                   targetLine);
            printf("CALL: Salvou %d na posição %d da pilha\n",
                   mvd.stack[mvd.stackPointer],
                   mvd.stackPointer);
            mvd.programCounter = (targetLine - 1) * 3;
            break;
        }

        case CME: {
            printf("Executando CME: %d < %d\n", mvd.stack[mvd.stackPointer-1], mvd.stack[mvd.stackPointer]);
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] < mvd.stack[mvd.stackPointer]) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;  // Faltava avançar o contador
            break;
        }

        case INV: {
            printf("Executando INV\n");
            mvd.stack[mvd.stackPointer] = -mvd.stack[mvd.stackPointer];
            mvd.programCounter += 3;
            break;
        }

        case AND: {
            printf("Executando AND\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] == 1 && mvd.stack[mvd.stackPointer] == 1) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case OR: {
            printf("Executando OR\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] == 1 || mvd.stack[mvd.stackPointer] == 1) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case CMA: {
            printf("Executando CMA\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] > mvd.stack[mvd.stackPointer]) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case CEQ: {
            printf("Executando CEQ\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] == mvd.stack[mvd.stackPointer]) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case CDIF: {
            printf("Executando CDIF\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] != mvd.stack[mvd.stackPointer]) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case CMEQ: {
            printf("Executando CMEQ\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] <= mvd.stack[mvd.stackPointer]) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case CMAQ: {
            printf("Executando CMAQ\n");
            mvd.stack[mvd.stackPointer-1] = (mvd.stack[mvd.stackPointer-1] >= mvd.stack[mvd.stackPointer]) ? 1 : 0;
            mvd.stackPointer--;
            mvd.programCounter += 3;
            break;
        }

        case RETURN: {
            printf("RETURN: Linha atual: %d, Retornando para linha: %d\n",
                   mvd.programCounter/3 + 1,
                   mvd.stack[mvd.stackPointer]);
            int returnLine = mvd.stack[mvd.stackPointer];
            mvd.programCounter = (returnLine - 1) * 3;
            mvd.stackPointer--;
            break;
        }

        case HLT: {
            printf("Executando HLT\n");
            mvd.running = 0;
            mvd.programCounter += 3;  // Avança 3 posições
            break;
        }
    }

    printf("Stack pointer depois: %d\n", mvd.stackPointer);
    printf("Program counter depois: %d\n\n", mvd.programCounter);

    updateStackView();
    updateCodeHighlight();
}

int parseInstruction(const char* instr) {
    if(strlen(instr) <= 0) return -1;

    switch(instr[0]) {
        case 'A':
            if(strcmp(instr, "ADD") == 0) return ADD;
            if(strcmp(instr, "ALLOC") == 0) return ALLOC;
            if(strcmp(instr, "AND") == 0) return AND;
            break;

        case 'C':
            if(strcmp(instr, "CALL") == 0) return CALL;
            if(strcmp(instr, "CMA") == 0) return CMA;
            if(strcmp(instr, "CME") == 0) return CME;
            if(strcmp(instr, "CMAQ") == 0) return CMAQ;
            if(strcmp(instr, "CMEQ") == 0) return CMEQ;
            if(strcmp(instr, "CEQ") == 0) return CEQ;
            if(strcmp(instr, "CDIF") == 0) return CDIF;
            break;

        case 'D':
            if(strcmp(instr, "DIVI") == 0) return DIVI;
            if(strncmp(instr, "DALLOC", 6) == 0) return DALLOC;  // Usando strncmp para pegar "DALLO"
            break;

        case 'H':
            if(strcmp(instr, "HLT") == 0) return HLT;
            break;

        case 'I':
            if(strcmp(instr, "INV") == 0) return INV;
            break;

        case 'J':
            if(strcmp(instr, "JMP") == 0) return JMP;
            if(strcmp(instr, "JMPF") == 0) return JMPF;
            break;

        case 'L':
            if(strcmp(instr, "LDC") == 0) return LDC;
            if(strcmp(instr, "LDV") == 0) return LDV;
            break;

        case 'M':
            if(strcmp(instr, "MULT") == 0) return MULT;
            break;

        case 'N':
            if(strcmp(instr, "NEG") == 0) return NEG;
            if(strcmp(instr, "NULL") == 0) return NULL_;
            break;

        case 'O':
            if(strcmp(instr, "OR") == 0) return OR;
            break;

        case 'P':
            if(strcmp(instr, "PRN") == 0) return PRN;
            break;

        case 'R':
            if(strcmp(instr, "RD") == 0) return RD;
            if(strncmp(instr, "RETURN", 6) == 0) return RETURN;  // Usando strncmp para pegar "RETUR"
            break;

        case 'S':
            if(strcmp(instr, "START") == 0) return START;
            if(strcmp(instr, "SUB") == 0) return SUB;
            if(strcmp(instr, "STR") == 0) return STR;
            break;
    }
    return -1;
}

void updateStackView() {
    ListView_DeleteAllItems(hwndMemoryList);

    for(int i = 0; i <= mvd.stackPointer; i++) {
        LVITEM lvi = {0};
        char addr[32], value[32];

        sprintf(addr, "%d", i);
        sprintf(value, "%d", mvd.stack[i]);

        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;

        lvi.iSubItem = 0;
        lvi.pszText = addr;
        ListView_InsertItem(hwndMemoryList, &lvi);

        lvi.iSubItem = 1;
        lvi.pszText = value;
        ListView_SetItem(hwndMemoryList, &lvi);
    }
}

void updateCodeHighlight() {
    int currentLine = mvd.programCounter / 3;

    // Força a seleção da linha atual
    SendMessage(hwndCodeList, WM_SETFOCUS, 0, 0);
    ListView_SetItemState(hwndCodeList, currentLine, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    // Atualiza a setinha
    for(int i = 0; i < ListView_GetItemCount(hwndCodeList); i++) {
        LVITEM item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 5;
        item.pszText = (i == currentLine) ? "<-" : "";
        ListView_SetItem(hwndCodeList, &item);
    }

    ListView_EnsureVisible(hwndCodeList, currentLine, TRUE);
}

void onRun(HWND hwnd) {
    if(!mvd.running) return;

    if(mvd.stepMode) {
        executeInstruction(hwnd, labelMap);
    } else {
        while(mvd.running) {
            executeInstruction(hwnd, labelMap);
        }
    }
}


void initMVD() {
    mvd.stackPointer = -1;
    mvd.programCounter = 0;
    mvd.running = 1;
    mvd.stepMode = 0;


    memset(mvd.stack, 0, sizeof(mvd.stack));
    memset(mvd.code, 0, sizeof(mvd.code));
    memset(labelMap, 0, sizeof(labelMap));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                  LPSTR lpCmdLine, int nCmdShow) {
    setlocale(LC_ALL, "Portuguese_Brazil.1252");
    SetConsoleOutputCP(1252);
    AllocConsole();
    system("chcp 65001");  // UTF-8
    WNDCLASSEX wc = {0};

    HWND hwnd;
    MSG msg;

    HBRUSH hBrush = CreateSolidBrush(RGB(173, 216, 230)); // Azul claro
    RegisterClassEx(&wc);
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush);

    // Inicializar common controls
    INITCOMMONCONTROLSEX icex = {0};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Registrar a classe da janela
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = className;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Falha ao registrar a classe da janela!",
                  "Erro!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Criar a janela principal
    hwnd = CreateWindowEx(
        0,
        className,
        "Simulador",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Falha ao criar a janela!",
                  "Erro!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Loop de mensagens
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            {
                InitializeUI(hwnd);
            }
            break;

        case WM_SIZE:
            ResizeWindow(hwnd);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_ARQUIVO_ABRIR:
                    loadMVDFile(hwnd);
                    break;

                case ID_SOBRE:
                    MessageBox(hwnd,
                             "Importe seu arquivo .OBJ e selecione sua modalidade de execução.\nPara a função 'Read' (RD), observe a caixa para input, digite, pressione ENTER e continue com a execução normalmente.",
                             "Sobre | Como Utilizar", MB_OK | MB_ICONINFORMATION);
                    break;

                case ID_BTN_RUN:
                    mvd.stepMode = SendMessage(hwndModeCombo, CB_GETCURSEL, 0, 0);
                    onRun(hwnd);
                    break;

                case ID_BTN_STOP:
                initMVD();
                ListView_DeleteAllItems(hwndCodeList);
                ListView_DeleteAllItems(hwndMemoryList);
                SetWindowText(hwndOutput, "");
                break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void InitializeUI(HWND hwnd) {
    // Criar o menu
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();

    AppendMenu(hFileMenu, MF_STRING, ID_ARQUIVO_ABRIR, "Abrir");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "Arquivo");
    AppendMenu(hMenu, MF_STRING, ID_SOBRE, "Sobre");

    SetMenu(hwnd, hMenu);

    // Criar os controles
    CreateCodeList(hwnd);
    CreateMemoryList(hwnd);
    CreateControls(hwnd);
}

void CreateCodeList(HWND hwnd) {
   RECT rect;
   GetClientRect(hwnd, &rect);

   hwndCodeList = CreateWindowEx(
       WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
       WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
       10, 10, (rect.right * 7) / 10 - 20, rect.bottom - 200,
       hwnd, (HMENU)ID_CODE_LIST, GetModuleHandle(NULL), NULL);

   ListView_SetExtendedListViewStyle(hwndCodeList,
    LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   LVCOLUMN lvc = {0};
   lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

   char *cols[] = {"Linha", "Instrução", "Atributo 1", "Atributo 2", "Comentário", ""};
   int widths[] = {50, 100, 100, 100, 200, 30};

   for (int i = 0; i < 6; i++) {
       lvc.iSubItem = i;
       lvc.pszText = cols[i];
       lvc.cx = widths[i];
       ListView_InsertColumn(hwndCodeList, i, &lvc);
   }
}

void CreateMemoryList(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Criar frame para memória
    CreateWindow("BUTTON", "Pilha / Memória",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        (rect.right * 7) / 10 + 10, 10,
        (rect.right * 3) / 10 - 30, rect.bottom - 200,
        hwnd, NULL, GetModuleHandle(NULL), NULL);

    // Criar lista de memória
    hwndMemoryList = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        (rect.right * 7) / 10 + 20, 30,
        (rect.right * 3) / 10 - 50, rect.bottom - 230,
        hwnd, (HMENU)ID_MEMORY_LIST, GetModuleHandle(NULL), NULL);

    // Adicionar colunas
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    char *cols[] = {"Endereço", "Valor"};
    int widths[] = {80, 80};

    for (int i = 0; i < 2; i++) {
        lvc.iSubItem = i;
        lvc.pszText = cols[i];
        lvc.cx = widths[i];
        ListView_InsertColumn(hwndMemoryList, i, &lvc);
    }
}

void CreateControls(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Criar área de saída
    hwndOutput = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
        10, rect.bottom - 180, rect.right - 300, 150,
        hwnd, (HMENU)ID_OUTPUT, GetModuleHandle(NULL), NULL);

    // Criar combo de modo
    hwndModeCombo = CreateWindow(
        "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        rect.right - 280, rect.bottom - 180, 120, 200,
        hwnd, (HMENU)ID_COMBO_MODE, GetModuleHandle(NULL), NULL);

    SendMessage(hwndModeCombo, CB_ADDSTRING, 0, (LPARAM)"Normal");
    SendMessage(hwndModeCombo, CB_ADDSTRING, 0, (LPARAM)"Passo a Passo");
    SendMessage(hwndModeCombo, CB_SETCURSEL, 0, 0);

    // Criar botões
    hwndStopBtn = CreateWindow(
        "BUTTON", "Parar",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rect.right - 280, rect.bottom - 140, 120, 30,
        hwnd, (HMENU)ID_BTN_STOP, GetModuleHandle(NULL), NULL);

    hwndRunBtn = CreateWindow(
        "BUTTON", "Executar",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rect.right - 280, rect.bottom - 100, 120, 30,
        hwnd, (HMENU)ID_BTN_RUN, GetModuleHandle(NULL), NULL);
}

void ResizeWindow(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Lista de código
    MoveWindow(hwndCodeList,
        10, 10,
        (rect.right * 7) / 10 - 20,
        rect.bottom - 200,
        TRUE);

    // Frame e lista de memória
    HWND hwndFrame = GetDlgItem(hwnd, 0);
    MoveWindow(hwndFrame,
        (rect.right * 7) / 10 + 10, 10,
        (rect.right * 3) / 10 - 30,
        rect.bottom - 200,
        TRUE);

    MoveWindow(hwndMemoryList,
        (rect.right * 7) / 10 + 20, 30,
        (rect.right * 3) / 10 - 50,
        rect.bottom - 230,
        TRUE);

    // Controles inferiores
    MoveWindow(hwndOutput,
        10, rect.bottom - 180,
        rect.right - 300, 150,
        TRUE);

    MoveWindow(hwndModeCombo,
        rect.right - 280, rect.bottom - 180,
        120, 200,
        TRUE);

    MoveWindow(hwndStopBtn,
        rect.right - 280, rect.bottom - 140,
        120, 30,
        TRUE);

    MoveWindow(hwndRunBtn,
        rect.right - 280, rect.bottom - 100,
        120, 30,
        TRUE);
}


# Compilador-Didatico
Compilador Didático desenvolvido como Projeto Final da disciplina de Desenvolvimento de Compiladores

===== READ ME =====

Como utilizar o compilador e a máquina virtual:

Baixe os arquivos Compilador.c e MaquinaVirtual.c bem como os executáveis.

Executável do Compilador: Utilize a aba 'arquivo' para importar um arquivo diretamente de seu computador. Certifique-se de que este arquivo é do tipo .txt. O código do seu programa então será apresentado na janela interativa, onde você pode editar o código e, quando desejar, clicar na opção 'Compilar', no canto inferior direito. Ao compilar, a janela de baixo irá indicar eventuais erros, ou os estados da sua compilação. Caso a compilação seja efetiva, a mensagem ao final será '=== Fim da Compilação ==='. Caso a compilação do seu programa detecte algum erro, a mensagem ao final será '=== Compilação encerrada por erro ===' e o primeiro erro que causou a falha será indicado na janela. Clique no erro e o código indicará exatamente onde ocorreu o erro, levando o cursor textual até lá e grifando em azul por dois segundos. Caso o compilador obtenha sucesso na compilação, um programa do tipo .OBJ será gerado com o nome de 'PROGRAMA.OBJ' no mesmo diretório de onde está o executável do compilador.

Executável da Máquina Virtual: Na janela da máquina virtual, com o título 'Simulador', utilize a aba 'arquivo' para importar um arquivo diretamente de seu computador. Certifique-se de que este arquivo é do tipo .OBJ. Neste ponto, o arquivo gerado pelo compilador 'Programa.OBJ' pode ser utilizado. Após a importação, podem ser utilizadas as funções 'Normal' e 'Passo a Passo' para decidir como será executada a máquina virtual. 'Normal' irá executar o programa todo diretamente, avançando até o próximo estado possível, seja ele a necessidade de um input do usuário ou o resultado final do programa. Já a modalidade 'Passo a Passo' irá executar uma instrução de cada vez, indicando no lado direito da janela o estado atual da pilha a cada instrução. Confira a saída da execução na janela inferior. Observação pertinente: para inputs, insira o cursor na caixa de texto que surge abaixo do botão 'Executar', digite seu input, pressione ENTER e continue a execução normal.

Observação: Se desejar manipular o código, utilizando-o como um projeto no CodeBlocks certifique-se de linkar as bibliotecas necessárias:

CodeBlocks, vá para Project -> Build Options -> Linker Settings e adicione:

gdi32
user32
kernel32
comctl32
comdlg32

Desenvolvido por:
Eduardo Augusto Marras de Souza
André Luís Bianchini Filho
João Gabriel de Campos Rosa

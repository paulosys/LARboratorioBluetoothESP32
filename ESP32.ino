#define PORTA 32 //Selecione a porta da ESP que será lida.
#define pontosTela 3000 //Quantidade de amostras necessarias para cobrir uma tela.

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

float dadosEnviar[pontosTela]; //Vetor que recebe a leitura da Porta Analogica da ESP.

float valorTrigger = 1.5;
int hz = 0, fase = 0;
float ampli = 0.0;
char dadosRecebidos[13]; //Dados recebidos pelo Bluetooth.
char dadosBluetooth[3][5]; //Matriz para colocar os dados recebidos em suas respectiveis variaveis.

bool sen = false, quad = false, tri = false, dser = false, oscilo = false, periodo = false;

void limpar() { //Função para limpar os dados anteriores do Bluetooth.
  for (int i = 0; i < 13; i++)dadosRecebidos[i] = {};
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 5; j++)
      dadosBluetooth[i][j] = ' ';
}


void setup()
{
  Serial.begin(115200);//Inicia a comunicaçao serial
  SerialBT.begin("Larboratorio Bluetooth"); //Nome do Bluetooth da ESP32
  xTaskCreatePinnedToCore(comunicacaoBluetooth, "comunicacaoBluetooth", 8192, NULL, 1, NULL, 0);//Cria a tarefa "comunicacaoBluetooth()" com prioridade 1, atribuída ao core 0
  delay(500);
}

void loop()//O loop() sempre será atribuído ao core 1 automaticamente pelo sistema, com prioridade 1
{
  if (oscilo == true) { //Quando o ociloscópio é ligado começo a enviar pelo Bluetooth
      
    int comparacaoTriggerAntes = -1;                                                //
    for(int i = 0; i < pontosTela; i++){                                            //Ciclo de no máximo uma tela do app
      int comparacaoTrigger  = (analogRead(PORTA)* 3.3 / 4095 > valorTrigger);      //Comparo se o valor atual é maior que o valor do trigger
      if ((comparacaoTriggerAntes == 0) and (comparacaoTrigger == 1)) break;        //Se for, começo a armazenar a onda no proximo for e saio do loop
      comparacaoTriggerAntes = comparacaoTrigger;                                   
      }
   
    for (int i = 0; i < pontosTela; i++) dadosEnviar[i] = analogRead(PORTA)* 3.3 / 4095; //Armazeno as leituras no vetor
    
    for (int i = 0; i < pontosTela; i++) SerialBT.println(dadosEnviar[i]); //Envio o vetor pelo Bluetooth
  }
}

void comunicacaoBluetooth(void*z)//Atribuímos a comunicacaoBluetooth ao core 0, com prioridade 1
{
  while (true) //Mantem o núcleo ativo para sempre.
  {
    if ((SerialBT.available() > 0)) { //Verifica se tem algo na Serial do Bluetooth.
      int cont = 0;
      limpar();
      while (SerialBT.available() > 0) { //Enquanto tiver dados deixa aberto e adiciona no vetor dadosRecebidos.
        delay(10);
        dadosRecebidos[cont] = SerialBT.read();
        cont++;
      }
      Serial.println(dadosRecebidos);
      if (dadosRecebidos[0] == '{') { //Verifico se os dados contem '{' no inicio, para configuração de dados.
        sen = false, quad = false, tri = false, dser = false, periodo = false; //Reseto as formas de onda para false.
        if (dadosRecebidos[1] == 's') sen = true; //Onda Senoidal.
        if (dadosRecebidos[1] == 'q') quad = true; //Onda Quadrada.
        if (dadosRecebidos[1] == 't') tri = true; //Onda Triangular.
        if (dadosRecebidos[1] == 'd') dser = true; //Onda Dente de Serra.
        if (dadosRecebidos[1] == 'i') oscilo = !oscilo; //Liga e desliga o Ociloscópio.
        if (dadosRecebidos[1] == 'z'){ //Defino o valor do trigger
          char valorTriggerConverter[3];
          for(int i = 2; i<=4; i++){
            valorTriggerConverter[i-2] = dadosRecebidos[i];
          }
            valorTrigger = atof(valorTriggerConverter);
        }
      }
      
      else {
        for (int i = 0, lin = 0, col = 0; dadosRecebidos[i] != '\0'; i++, col++) { //Converto os valores que recebi e coloco numa matriz.
          if (dadosRecebidos[i] == '/')lin++, col = 0;
          else dadosBluetooth[lin][col] = dadosRecebidos[i];
        }

        hz = atoi(dadosBluetooth[0]); //Tiro da matriz, converto para inteiro e coloco na variável.
        ampli = atof(dadosBluetooth[1]); //Tiro da matriz, converto para float e coloco na variável.
        fase = atoi(dadosBluetooth[2]); //Tiro da matriz, converto para inteiro e coloco na variável.
      }
    }
    delay(10);
  }
}

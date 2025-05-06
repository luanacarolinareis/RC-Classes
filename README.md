# 📡 RC-Classes

Projeto de Redes de Comunicação - Sistema de turmas online para difusão de conteúdos, recorrendo a diversas técnicas de comunicação e com recurso aos protocolos da pilha TCP/IP.

---

## 🚀 Sobre o Projeto

O **RC-Classes** é um sistema online de turmas que permite a gestão e comunicação entre utilizadores (alunos, professores e administradores) numa rede simulada, explorando técnicas de comunicação e protocolos da pilha TCP/IP. O projeto foi desenvolvido no âmbito da unidade curricular de Redes de Comunicação.

---

## ✨ Funcionalidades

- 👩‍🏫 Criação e gestão de turmas online
- 📚 Partilha e envio de conteúdos via multicast
- 💬 Comunicação entre utilizadores autenticados (aluno, professor, administrador)
- 🔒 Autenticação de utilizadores com diferentes permissões
- 🔧 Administração remota do sistema

---

## 🛠️ Tecnologias Utilizadas

- Stack TCP/IP
- Técnicas de comunicação em redes
- Configuraçao de rede no GNS3

---

## ⚙️ Como Executar

### 1. Clone este repositório:
- git clone https://github.com/luanacarolinareis/RC-Classes.git

### 2. Compilar os programas
- Os comandos de compilação e execução encontram-se detalhados no ficheiro [`compile-run.txt`](compile-run.txt)[1].  
- Resumo dos passos principais:
gcc -o class_server class_server.c
gcc -pthread -o class_client class_client.c
gcc -o admin_client admin_client.c

### 3. Executar o servidor

- ./class_server 6000 5000 config.txt

### 4. Executar os clientes

- **Cliente (Aluno/Professor):**
  ./class_client 193.137.100.1 6000

- **Cliente Administrador:**
  ./admin_client 193.137.100.1 5000

### 5. Configuração de utilizadores

- O ficheiro [`config.txt`](config.txt)[2] contém os utilizadores registados no sistema, no formato: nome;password;tipo.

### 6. Configuração de rede e routers

- Os ficheiros [`config-commands.txt`](config-commands.txt)[3] e o relatório detalham a configuração de rede dos clientes, servidor e routers, incluindo endereçamentos IP, gateways e comandos para os routers, essenciais para o funcionamento do sistema numa rede simulada.

---

## 📑 Documentação

- 📄 **Relatório Final:**  
  O funcionamento detalhado do sistema, exemplos de utilização, arquitetura da rede, configuração dos routers e containers, bem como as funcionalidades implementadas para cada tipo de utilizador, estão descritos no [`relatorio-final.pdf`](relatorio-final.pdf)[4].

- 📝 **Configuração e Execução:**  
  Consulte [`compile-run.txt`](compile-run.txt)[1] para instruções rápidas de compilação e execução, e [`config-commands.txt`](config-commands.txt)[3] para exemplos de configuração de rede e routers.

---

## 📸 Capturas de Ecrã

![image](https://github.com/user-attachments/assets/e3c2e1dc-85cc-477f-98cd-14f2fc41e7ae)
![image](https://github.com/user-attachments/assets/fbbe461d-6726-4eea-95ee-09858fec1f2a)
![image](https://github.com/user-attachments/assets/c0483f69-c89b-4bd2-86a3-2611872675ad)
![image](https://github.com/user-attachments/assets/faf93fb2-4cb0-4f2a-a9ba-617ead4f7041)

---

## 👩‍💻 Autoras

- [Carolina Reis](https://github.com/luanacarolinareis)
- Gabriela Mendoza

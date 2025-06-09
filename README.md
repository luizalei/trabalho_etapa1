# Sistema de Monitoramento Ferroviário com Detecção de Rodas Quentes

## Introdução

Este projeto foi desenvolvido como Trabalho Final da disciplina **ELT127 - Automação em Tempo Real** da Universidade Federal de Minas Gerais (UFMG), ministrada pelo professor **Luiz Themystokliz S. Mendes**. O objetivo do projeto é simular, em ambiente computacional, um sistema de monitoramento ferroviário em tempo real, com ênfase na **detecção de rodas quentes** — condição crítica que pode levar ao descarrilamento de composições ferroviárias.

O sistema é composto por múltiplas tarefas (threads/processos) que se comunicam e se sincronizam entre si para realizar funções como leitura de sensores, exibição em terminais, detecção de falhas e interação com o operador via teclado.

## Descrição do Projeto

A aplicação simula um sistema real de controle ferroviário composto por:

- Leitura periódica de **dados de sinalização** e **detectores de rodas quentes** de CLPs;
- Armazenamento dos dados em uma **lista circular na memória RAM**;
- Encaminhamento das informações para:
  - **Arquivos circulares em disco** (no caso da sinalização);
  - **Console de visualização de rodas quentes**, em caso de falhas ou alertas;
- Visualização em **dois terminais distintos**:
  - Um para exibir os dados de sinalização ferroviária;
  - Outro para exibir informações sobre rodas quentes;
- Controle via **teclado**, permitindo pausar e retomar tarefas, ou encerrar toda a aplicação;
- Implementação de **IPC (Inter-Process Communication)**, mecanismos de **sincronização** e temporizações com precisão, sem uso da função `Sleep()`.

O projeto foi dividido em duas etapas:
- **Etapa 1**: Implementação da arquitetura multitarefa e lista circular na RAM.
- **Etapa 2**: Integração completa, com temporizações corretas, IPC, manipulação de arquivos circulares em disco e testes finais.

## Tecnologias Utilizadas

- Linguagem: **C++**
- Ambiente de desenvolvimento: **Microsoft Visual Studio Community Edition**
- API: **Win32**
- Técnicas: Threads, Eventos, Pipes/Mailslots, Mutexes, Manipulação de arquivos, Temporização sem `Sleep()`, Comunicação interprocessos.

## Autoras

- **Camila Chagas Carvalho**
- **Luiza Calheiros Lei**

*Curso de Engenharia Eletrônica - Universidade Federal de Minas Gerais (UFMG)*

---


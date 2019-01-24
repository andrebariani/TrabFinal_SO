/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096

unsigned short fat[65536];

typedef struct {
       char used;
       char name[25];
       unsigned short first_block;
       int size;
} dir_entry;

typedef struct {
	char estado;
   	int posAtual;
} Arquivo;

dir_entry dir[128];
Arquivo arquivos[128];

/* Constantes de Agrupamentos */

#define AGRUP_LIVRE 1
#define AGRUP_ULTIMO 2
#define AGRUP_FAT 3
#define AGRUP_DIR 4
#define SIZE_FAT 65536
#define SIZE_DIR 128

/* Constantes de Arquivos */
#define ARQ_FECHADO 'C'
#define ARQ_ABERTO_ESCRITA 'W'
#define ARQ_ABERTO_LEITURA 'R'

int fs_init() {
  //Carregando FAT
  for(int agrupamento = 0; agrupamento < 32; agrupamento++)
  {
      for(int sector = 0; sector < 8; sector++)
      {
        char buffer[SECTORSIZE];
        if(!bl_read((agrupamento*8 + sector), buffer))
        {
            printf("Erro no carregamento da FAT. Disco nao esta formatado!\n");
            return 0;
        }
        memcpy(((char*) fat)+((agrupamento*8+sector)*SECTORSIZE), buffer, SECTORSIZE);
      }
  }
  //Verficando integridade
  for(int i = 0; i < 32; i++)
  {
    if(fat[i] != AGRUP_FAT)
    {
        printf("Erro no carregamento da FAT. Disco nao esta formatado!\n");
        return 1;
    }
  }
  if(fat[32] != AGRUP_DIR)
  {
      printf("Erro no carregamento do FAT. Disco nao esta formatado!\n");
      return 1;
  }

  //Carregando Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];

    if(!bl_read(sector + 32*8, buffer)){
        printf("Erro no carregamento da Diretorio. Disco nao esta formatado!\n");
        return 0;
    }
    memcpy(((char*) dir)+sector*SECTORSIZE, buffer, SECTORSIZE);

  }

  for(int i = 0 ; i < SIZE_DIR ; i++)
      if(dir[i].used == 'T')
        arquivos[i].estado = ARQ_FECHADO;

  return 1;
}

int fs_format() {

  //Inicializando estruturas em RAM
  //FAT
  for(int i = 0; i < 32; i++)
  {
    fat[i] = AGRUP_FAT;
  }
  fat[32] = AGRUP_DIR;
  for(int i = 33; i < SIZE_FAT; i++)
  {
    fat[i] = AGRUP_LIVRE;
  }

  //Diretório
  for(int i = 0; i < 128; i++)
  {
    dir[i].used = 'F';
    dir[i].name[0] = '\0';
  }

  //Escrevendo no arquivo
  //FAT
  for(int agrupamento = 0; agrupamento < 32; agrupamento++)
  {
    for(int sector = 0; sector < 8; sector++)
    {
      char buffer[SECTORSIZE];
      memcpy(buffer, ((char*) fat)+((agrupamento*8 + sector)*SECTORSIZE), SECTORSIZE);
      bl_write((agrupamento*8 + sector), buffer);
    }
  }

  //Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];
    memcpy(buffer, ((char*) dir)+sector*SECTORSIZE, SECTORSIZE);
    bl_write(sector + 32*8, buffer);

  }

  return 1;
}

int fs_free() {
  int agrupOcup = 0;

  //Contando os espaços livres da FAT
  for (int i = 0; i < SIZE_FAT; i++)
  {

    if(fat[i] != AGRUP_LIVRE)
    {
      agrupOcup++;
    }
  }

  //Multiplicando para tranformar os clusters em bytes
  return (bl_size()-agrupOcup* 8)*SECTORSIZE;
}

int fs_list(char *buffer, int size) {
  buffer[0] = '\0';
  char aux[31];

  for (int i = 0; i < SIZE_DIR; i++)
  {
    if(dir[i].used == 'T')
    {
      sprintf(aux, "%s\t\t%d\n", dir[i].name, dir[i].size);
      strcpy( &( buffer[strlen(buffer)] ), aux);
    }
  }

  return 1;
}

int fs_create(char* file_name) {

    //Testando tamanho do nome
    if(strlen(file_name)>24)
    {
        printf("Erro: Nome de arquivo muito grande!\n");
        return 0;
    }

    int entradaDirLivre=-1;
    //Buscando arquivo no diretorio
    for (int i = 0; i < SIZE_DIR; i++)
    {
      if(dir[i].used == 'T' && !strcmp(file_name, dir[i].name))
      {
          printf("Erro: Ja existe um arquivo com esse nome!\n");
          return 0;
      }
      else if(dir[i].used == 'F' && entradaDirLivre == -1 )
      {
          entradaDirLivre = i;
      }

    }

    if(entradaDirLivre == -1) {
        printf("Erro: Diretorio cheio!\n");
        return 0;
    }

    //Buscando agrupamento livre na FAT
    int posFat=33;

    while(posFat < SIZE_FAT&&fat[posFat]!=AGRUP_LIVRE)
        posFat++;

    if(posFat==SIZE_FAT)
    {
        printf("Erro: Nao ha espaco livre no disco!\n");
        return 0;
    }

    //Definindo valores

    //Diretorio
    dir[entradaDirLivre].used='T';
    strncpy(dir[entradaDirLivre].name,file_name,25);
    dir[entradaDirLivre].first_block=posFat;
    dir[entradaDirLivre].size=0;
    // estado do arquivo
    arquivos[entradaDirLivre].estado=ARQ_FECHADO;
    //FAT
    fat[posFat]=AGRUP_ULTIMO;

  //Escrevendo no arquivo
  //FAT
  for(int agrupamento = 0; agrupamento < 32; agrupamento++)
  {
    for(int sector = 0; sector < 8; sector++)
    {
      char buffer[SECTORSIZE];
      memcpy(buffer, ((char*) fat)+((agrupamento*8 + sector)*SECTORSIZE), SECTORSIZE);
      bl_write((agrupamento*8 + sector), buffer);
    }
  }

  //Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];
    memcpy(buffer, ((char*) dir)+sector*SECTORSIZE, SECTORSIZE);
    bl_write(sector + 32*8, buffer);

  }

    return 1;
}

int fs_remove(char *file_name) {

  int indice = -1;
  int i;
  for(i = 0; i < SIZE_DIR; i++)
  {
    //Checando nome e disponibilidade
    if(dir[i].used == 'T' && !strcmp(file_name, dir[i].name))
    {
        indice = dir[i].first_block;
        break;
    }
  }

  if(indice == -1)
  {
    printf("Erro: Arquivo inexistente!\n");

    return 0;
  }

  //Removendo o arquivo
  int anterior = indice;
  while(fat[indice] != AGRUP_ULTIMO)
  {
    indice = fat[anterior];
    fat[anterior] = AGRUP_LIVRE;
    anterior = indice;
  }

  fat[indice] = AGRUP_LIVRE;
  dir[i].used = 'F';

  //Escrevendo no arquivo
  //FAT
  for(int agrupamento = 0; agrupamento < 32; agrupamento++)
  {
    for(int sector = 0; sector < 8; sector++)
    {
      char buffer[SECTORSIZE];
      memcpy(buffer, ((char*) fat)+((agrupamento*8 + sector)*SECTORSIZE), SECTORSIZE);
      bl_write((agrupamento*8 + sector), buffer);
    }
  }

  //Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];
    memcpy(buffer, ((char*) dir)+sector*SECTORSIZE, SECTORSIZE);
    bl_write(sector + 32*8, buffer);

  }

  return 1;
}

int fs_open(char *file_name, int mode) {
  printf("fs_open\n");
	//Testando tamanho do nome
  if(strlen(file_name)>24)
  {
	printf("Erro: Nome de arquivo muito grande!\n");
    return -1;
	}
	//Buscando arquivo no diretorio
	int pos=0;
	while(pos < SIZE_DIR && dir[pos].used == 'T' && strcmp(file_name, dir[pos].name))
        pos++;


	if(mode == FS_R)
	{
        printf("In position %d\n", pos);
        printf("dir: %c\n", dir[pos].used);
        printf("arquivos: %c\n", arquivos[pos].estado);
		//Leitura
		if(pos==SIZE_DIR)
		{
			printf("Erro: Arquivo %s nao existe!\n", file_name);
    	return -1;
		}

		if(arquivos[pos].estado==ARQ_FECHADO){
			arquivos[pos].estado = ARQ_ABERTO_LEITURA;
			arquivos[pos].posAtual = 0;
		}
		else
		{
			printf("Erro: Arquivo %s ja esta aberto!\n", file_name);
    	return -1;
		}
	}
	else
	{
		//Escrita
		if(pos==SIZE_DIR)
		{
			if(!fs_create(file_name))
			{
				return -1;
			}

			pos=0;
			while(pos < SIZE_DIR && dir[pos].used == 'T' && strcmp(file_name, dir[pos].name))
			         pos++;

		}

        printf("In position %d\n", pos);
        printf("dir: %c\n", dir[pos].used);
        printf("arquivos: %c\n", arquivos[pos].estado);

		if(arquivos[pos].estado==ARQ_FECHADO){
			arquivos[pos].estado = ARQ_ABERTO_ESCRITA;
			arquivos[pos].posAtual = 0;
		}
		else
		{
			printf("Erro: Arquivo %s ja esta aberto!\n", file_name);
    	return -1;
		}
	}

  return pos;
}

int fs_close(int file)  {
  printf("fs_close\n");

	if(arquivos[file].estado==ARQ_FECHADO){
		printf("Erro: Arquivo ja esta fechado!\n");
    	return 0;
	}
	else
	{
		arquivos[file].estado = ARQ_FECHADO;
		arquivos[file].posAtual = -1;
    }
  return 1;
}

int fs_write(char *buffer, int size, int file) {

  if(arquivos[file].estado==ARQ_ABERTO_LEITURA)
  {
      printf("Erro: Arquivo aberto para leitura!\n");
      return -1;
  }
  else if(arquivos[file].estado==ARQ_FECHADO){
      printf("Erro: Arquivo nao foi aberto!\n");
      return -1;
  }

  //Verificando se existe espaço no disco para escrita
  //Calculando tamanho total final do arquivos (em bytes)
  int tamFinal= dir[file].size + size;

   //Convertendo o tamanho em blocos
  if(tamFinal % CLUSTERSIZE)
  {
      tamFinal = (tamFinal / CLUSTERSIZE) + 1;
  }
  else
  {
      tamFinal = tamFinal / CLUSTERSIZE;
  }

  int agrupAtual = dir[file].first_block;

  tamFinal--;

  //Pulando agrupamentos ja escritos
  while (fat[agrupAtual]!=AGRUP_ULTIMO) {
      agrupAtual=fat[agrupAtual];
      tamFinal--;
  }

  int agrupFinalOriginal = agrupAtual;

  //Enquanto houverem agrup. a serem escritos
  while (tamFinal>0) {
      //Buscando agrupamento livre na FAT
      int posFat=33;

      while(posFat < SIZE_FAT&&fat[posFat]!=AGRUP_LIVRE)
          posFat++;

      if(posFat==SIZE_FAT)
      {
          printf("Erro: Nao ha espaco livre no disco!\n");
          return -1;
      }

      fat[agrupAtual]=posFat;
      fat[posFat]=AGRUP_ULTIMO;
      agrupAtual=posFat;
      tamFinal--;
  }

  //Escrevendo (EFETIVAMENTE) dados no disco
  //Carregando o ultimo agrupamento
  char agrupBuffer[CLUSTERSIZE];

  for(int sector = 0; sector < 8; sector++)
  {
    if(!bl_read((agrupFinalOriginal*8 + sector), (char *) agrupBuffer + sector*SECTORSIZE))
    {
        printf("Erro no carregamento do arquivo.\n");
        return -1;
    }
  }

  printf("tam:%d\n", CLUSTERSIZE - (dir[file].size%CLUSTERSIZE));

  //Copiando o inicio do agrupamento novo no final do lido
  if(size>(CLUSTERSIZE - (dir[file].size%CLUSTERSIZE)))
    memcpy(((char *) agrupBuffer)+(dir[file].size%CLUSTERSIZE),(char *) buffer, CLUSTERSIZE - (dir[file].size%CLUSTERSIZE));
  else
    memcpy(((char *) agrupBuffer)+(dir[file].size%CLUSTERSIZE),(char *) buffer, size);

  printf("Agrupamento a ser escrito: %d\n", agrupFinalOriginal);
  //Escrevendo o primeiro cluster
  for(int sector = 0; sector < 8; sector++)
  {
      if(!bl_write((agrupFinalOriginal*8 + sector), (char *) (agrupBuffer) + sector*SECTORSIZE))
      {
          printf("Erro na escrita do arquivo.\n");
          return -1;
      }
  }
  //Corrigindo ponteiro do buffer para apontar para o próximo agrupamento
  buffer += (CLUSTERSIZE - (dir[file].size%CLUSTERSIZE));
  int clustersEscritos=1;
  //Escrevendo os demais clusters
  for(int cluster = fat[agrupFinalOriginal]; cluster != AGRUP_ULTIMO; cluster = fat[cluster]){

    for(int sector = 0; sector < 8; sector++)
    {
        if(!bl_write((cluster*8 + sector), (char *) buffer + clustersEscritos*CLUSTERSIZE + sector*SECTORSIZE))
        {
            printf("Erro na escrita do arquivo.\n");
            return -1;
        }
    }
    //Pulando o agrupamento escrito
    buffer += CLUSTERSIZE;
    clustersEscritos++;
  }

  //Atualizando tamanho do arquivo no diretório
  dir[file].size+=size;

  //Salvando estruturas no disco
  //Salvando FAT
  for(int cluster = 0; cluster < 32; cluster++)
  {
    for(int sector = 0; sector < 8; sector++)
    {
      char buffer[SECTORSIZE];
      memcpy(buffer, ((char*) fat)+((cluster*8 + sector)*SECTORSIZE), SECTORSIZE);
      bl_write((cluster*8 + sector), buffer);
    }
  }

  //Salvando Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];
    memcpy(buffer, ((char*) dir)+sector*SECTORSIZE, SECTORSIZE);
    bl_write(sector + 32*8, buffer);

  }

  return size;
}

int fs_read(char *buffer, int size, int file) {
  char bufferLeitura[SECTORSIZE];
  int agrup = dir[file].first_block;
  int tamanho, lido;

  printf("verificações... ");
  if(arquivos[file].estado==ARQ_ABERTO_ESCRITA)
  {
      printf("Erro: Arquivo aberto para escrita!\n");
      return -1;
  }
  else if(arquivos[file].estado==ARQ_FECHADO){
      printf("Erro: Arquivo nao foi aberto!\n");
      return -1;
  }

  printf("ok!\n");
  printf("tamanho... ");
  if(size < dir[file].size-arquivos[file].posAtual)
  {
    tamanho = size;
  }
  else
  {
    tamanho = dir[file].size-arquivos[file].posAtual;
  }
  printf("%d\n", tamanho);
  printf("ok!\n");

  //Primeiro agrupamento a ser lido
  for(int i = arquivos[file].posAtual; i > CLUSTERSIZE; i -= CLUSTERSIZE)
  {
    agrup = fat[agrup];
  }

  //Primeiro setor a ser lido
  bl_read(agrup*8 + (arquivos[file].posAtual / SECTORSIZE), bufferLeitura);

  //Leitura
  lido = 0;
  while(lido < tamanho && arquivos[file].posAtual < dir[file].size)
  {
    buffer[lido] = bufferLeitura[arquivos[file].posAtual % SECTORSIZE];
    arquivos[file].posAtual++;
    lido++;

    //Checando se acabou o agrupamento
    if(!(arquivos[lido].posAtual % CLUSTERSIZE))
    {
      agrup = fat[agrup];
    }

    //Checando se acabou o setor
    if(!(arquivos[file].posAtual % SECTORSIZE))
    {
      bl_read(agrup*8 + (arquivos[file].posAtual / SECTORSIZE), bufferLeitura);
    }
  }

  return lido;
}
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

dir_entry dir[128];

/* Constantes de Agrupamentos */

#define AGRUP_LIVRE 1
#define AGRUP_ULTIMO 2
#define AGRUP_FAT 3
#define AGRUP_DIR 4
#define SIZE_FAT 65536
#define SIZE_DIR 128

/* Funções auxiliaes */

int fs_init() {

  //Carregando FAT
  for(int cluster = 0; cluster < 32; cluster++)
  {
      for(int sector = 0; sector < 8; sector++)
      {
        char buffer[SECTORSIZE];
        if(!bl_read((cluster*8 + sector), buffer))
        {
            perror("Erro no carregamento da FAT. Disco nao formatado");
            return 0;
        }
        memcpy((char *) fat, buffer, SECTORSIZE);
      }
  }

  //Verficando integridade
  for(int i = 0; i < 32; i++)
  {
    if(fat[i] != AGRUP_FAT)
    {
        perror("Erro na integridade da FAT. Disco nao formatado");
        return 1;
    }
  }
  if(fat[32] != AGRUP_DIR)
  {
      perror("Erro na integridade Diretorio. Disco nao formatado");
      return 1;
  }

  //Carregando Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];

    if(!bl_read(sector + 256, buffer)){
        perror("Erro no carregamento do Diretorio. Disco nao formatado");
        return 0;
    }
    memcpy((char *) dir, buffer, SECTORSIZE);
  }

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
  for(int cluster = 0; cluster < 32; cluster++)
  {
    for(int sector = 0; sector < 8; sector++)
    {
      char buffer[SECTORSIZE];
      memcpy(buffer, (char *) fat, SECTORSIZE);
      bl_write((cluster*8 + sector), buffer);
    }
  }

  //Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];
    memcpy(buffer, (char *) dir, SECTORSIZE);
    bl_write(sector + 256, buffer);

  }

  return 1;
}

int fs_free() {
  int agrupOcup = 0;

  //Contando os empaços livres da FAT
  for (int i = 0; i < SIZE_FAT; i++)
  {
    if(fat[i] != AGRUP_LIVRE)
    {
      agrupOcup++;
    }
  }

  //Multiplicando para tranformar os clusters em bytes
  return (bl_size()-agrupOcup* CLUSTERSIZE)*SECTORSIZE;
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
        perror("Erro: Nome de arquivo muito grande!");
        return 0;
    }

    int entradaDirLivre=-1;
    //Buscando arquivo no diretorio
    for (int i = 0; i < SIZE_DIR; i++)
    {
      if(dir[i].used == 'T' && !strcmp(file_name, dir[i].name))
      {
          perror("Erro: Ja existe um arquivo com esse nome!");
          return 0;
      }
      else if(dir[i].used == 'F' && entradaDirLivre == -1 )
      {
          entradaDirLivre = i;
      }

    }

    //Buscando agrupamento livre na FAT
    int posFat=33;

    while(posFat < SIZE_FAT&&fat[posFat]!=AGRUP_LIVRE)
        posFat++;

    if(posFat==SIZE_FAT)
    {
        perror("Erro: Nao ha espaco livre no disco!");
        return 0;
    }

    //Definindo valores

    //Diretorio
    dir[entradaDirLivre].used='T';
    strncpy(dir[entradaDirLivre].name,file_name,25);
    dir[entradaDirLivre].first_block=posFat;
    dir[entradaDirLivre].size=0;
    //FAT
    fat[posFat]=AGRUP_ULTIMO;

    return 1;
}

int fs_remove(char *file_name) {
  printf("Função não implementada: fs_remove\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}

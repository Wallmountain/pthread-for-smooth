#include <iostream>
#include <string>
#include <time.h>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"
#include <pthread.h>

using namespace std;

// Shared
int thread_count = 0;
int counter[2];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int partition;
//times of smooth
#define NSmooth 1000

BMPHEADER bmpHeader;
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;
RGBTRIPLE **BMPData = NULL;

int readBMP( char *fileName); //read file
int saveBMP( char *fileName); //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X ); //allocate memory
void* smooth(void *);

int main(int argc,char **argv)
{
    char infileName[10] = "input.bmp";
    char outfileName[11] = "output.bmp";
    double startwtime = 0.0, endwtime=0;
 
    thread_count = atoi(argv[1]);
    if(thread_count == 0)
    {
        printf("error: 0 thread");
        return 0;
    }
    pthread_t t[thread_count];

    //start time
    startwtime = (double)clock() / CLOCKS_PER_SEC;
    //read file
    if ( readBMP( infileName) )
        cout << "Read file successfully!!" << endl;
    else 
        cout << "Read file fails!!" << endl;

    // give the other array space
    BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

    //change two array
    swap(BMPSaveData,BMPData);

    partition = bmpInfo.biHeight / thread_count;

    //using for busy waiting
    counter[0]=0;
    for(int i=0; i < thread_count; i++){
        int *c = (int *)malloc(sizeof(int));
        *c = i;
        pthread_create(&t[i], NULL, smooth, c);
    }

    for(int i=0; i < thread_count; i++)
        pthread_join(t[i], NULL);

    swap(BMPSaveData,BMPData);

    // save data
    if ( saveBMP( outfileName ) )
        cout << "Save file successfully!!" << endl;
    else
        cout << "Save file fails!!" << endl;

    // end time and print time
    endwtime =  (double)clock() / CLOCKS_PER_SEC;;
    cout << "The execution time = "<< endwtime-startwtime <<endl ;

    pthread_mutex_destroy(&mutex);
    free(BMPData[0]);
    free(BMPSaveData[0]);
    free(BMPData);
    free(BMPSaveData);

    return 0;
}

void* smooth(void *data)
{
    int *id = (int *) data;
    int Top, Down, Left, Right, i, j, k, count, end, first, up, down;
    RGBTRIPLE **SaveData = alloc_memory( partition + 2, bmpInfo.biWidth);
    RGBTRIPLE **Data = alloc_memory( partition + 2, bmpInfo.biWidth);

    first = partition * *id;
    end = partition * (*id+1);
    if(end > bmpInfo.biHeight) end = bmpInfo.biHeight;

    if(*id == 0) up = bmpInfo.biWidth;
    else up = first - 1;

    if(*id == thread_count - 1) down = 0;
    else down = end;

    for(i = first, k = 1; i<end; i++, k++)
    {
        for(j = 0; j<bmpInfo.biWidth; j++)
        {
            Data[k][j].rgbBlue = BMPData[i][j].rgbBlue;
	    Data[k][j].rgbRed = BMPData[i][j].rgbRed;
	    Data[k][j].rgbGreen = BMPData[i][j].rgbGreen;
        }
    }

    for(count = 0; count < NSmooth; count++)
    {
        for(j = 0; j<bmpInfo.biWidth; j++)
        {
            Data[0][j].rgbBlue = BMPData[up][j].rgbBlue;
            Data[0][j].rgbRed = BMPData[up][j].rgbRed;
            Data[0][j].rgbGreen = BMPData[up][j].rgbGreen;
            Data[partition + 1][j].rgbBlue = BMPData[down][j].rgbBlue;
            Data[partition + 1][j].rgbRed = BMPData[down][j].rgbRed;
            Data[partition + 1][j].rgbGreen = BMPData[down][j].rgbGreen;
        }
        for(i = 1; i <= partition; i++)
        {
            for(j = 0; j<bmpInfo.biWidth; j++)
            {
                Top = i-1;
                Down = i+1;
                Left = (j+bmpInfo.biWidth-1)%bmpInfo.biWidth;
                Right = (j+1)%bmpInfo.biWidth;

                SaveData[i][j].rgbBlue =  (double) (Data[i][j].rgbBlue+Data[Top][j].rgbBlue+Data[Top][Left].rgbBlue+Data[Top][Right].rgbBlue+Data[Down][j].rgbBlue+Data[Down][Left].rgbBlue+Data[Down][Right].rgbBlue+Data[i][Left].rgbBlue+Data[i][Right].rgbBlue)/9+0.5; 
                SaveData[i][j].rgbGreen =  (double) (Data[i][j].rgbGreen+Data[Top][j].rgbGreen+Data[Top][Left].rgbGreen+Data[Top][Right].rgbGreen+Data[Down][j].rgbGreen+Data[Down][Left].rgbGreen+Data[Down][Right].rgbGreen+Data[i][Left].rgbGreen+Data[i][Right].rgbGreen)/9+0.5; 
                SaveData[i][j].rgbRed =  (double) (Data[i][j].rgbRed+Data[Top][j].rgbRed+Data[Top][Left].rgbRed+Data[Top][Right].rgbRed+Data[Down][j].rgbRed+Data[Down][Left].rgbRed+Data[Down][Right].rgbRed+Data[i][Left].rgbRed+Data[i][Right].rgbRed)/9+0.5;  
            }
	}

 	for(j = 0; j<bmpInfo.biWidth; j++)
        {
            BMPData[first][j].rgbBlue = SaveData[1][j].rgbBlue;
            BMPData[first][j].rgbRed = SaveData[1][j].rgbRed;
            BMPData[first][j].rgbGreen = SaveData[1][j].rgbGreen;
	    BMPData[end - 1][j].rgbBlue = SaveData[partition][j].rgbBlue;
            BMPData[end - 1][j].rgbRed = SaveData[partition][j].rgbRed;
            BMPData[end - 1][j].rgbGreen = SaveData[partition][j].rgbGreen;
        }

        pthread_mutex_lock(&mutex);
        if(counter[count%2] == thread_count - 1)
            counter[(count+1)%2]=0;
        counter[count%2]++;
        pthread_mutex_unlock(&mutex);
        while(counter[count%2]<thread_count);

        swap(SaveData,Data);
    }

    for(i = first, k = 1; i<end; i++, k++)
    {
        for(j = 0; j<bmpInfo.biWidth; j++)
        {
            BMPData[i][j].rgbBlue = Data[k][j].rgbBlue;
            BMPData[i][j].rgbRed = Data[k][j].rgbRed;
            BMPData[i][j].rgbGreen = Data[k][j].rgbGreen;
        }
    }

    return NULL;
}

int readBMP(char *fileName)
{
    ifstream bmpFile( fileName, ios::in | ios::binary );

    if ( !bmpFile )
    {
        cout << "It can't open file!!" << endl;
        return 0;
    }

    bmpFile.read( (char *) &bmpHeader, sizeof( BMPHEADER ) );

    if( bmpHeader.bfType != 0x4d42 )
    {
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }
    bmpFile.read( (char *) &bmpInfo, sizeof( BMPINFO ) );
    
    if ( bmpInfo.biBitCount != 24 )
    {
        cout << "The file is not 24 bits!!" << endl;
        return 0;
    }

    while( bmpInfo.biWidth % 4 != 0 )
        bmpInfo.biWidth++;

    BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
    //for(int i = 0; i < bmpInfo.biHeight; i++)
    //bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));

    bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

    bmpFile.close();
    return 1;
}

int saveBMP( char *fileName)
{
    if( bmpHeader.bfType != 0x4d42 )
    {
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //建立輸出檔案物件
    ofstream newFile( fileName,  ios:: out | ios::binary );

    //檔案無法建立
    if ( !newFile )
    {
        cout << "The File can't create!!" << endl;
        return 0;
    }
	
    //寫入BMP圖檔的標頭資料
    newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

    //寫入BMP的資訊
    newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );
    //for( int i = 0; i < bmpInfo.biHeight; i++ )
    //newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
    newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

    newFile.close();
    return 1;
}

RGBTRIPLE **alloc_memory(int Y, int X )
{        
    RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
    RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
    memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );
    for( int i = 0; i < Y; i++)
        temp[ i ] = &temp2[i*X];

    return temp; 
}

void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
    RGBTRIPLE *temp;
    temp = a;
    a = b;
    b = temp;
}


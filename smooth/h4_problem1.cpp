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

    partition = bmpInfo.biWidth / thread_count;

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
    int Top, Down, Left, Right, i, j, count, end;
    
    end = partition * (*id+1);
    if(end > bmpInfo.biWidth) end = bmpInfo.biWidth;

    for(count = 0; count < NSmooth; count++)
    {
        for(i = 0; i<bmpInfo.biHeight; i++)
        {
            for(j = partition * *id; j<end; j++)
            {
                Top = (i+bmpInfo.biHeight-1)%bmpInfo.biHeight;
                Down = (i+1)%bmpInfo.biHeight;
                Left = (j+bmpInfo.biWidth-1)%bmpInfo.biWidth;
                Right = (j+1)%bmpInfo.biWidth;

                BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5; 
                BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5; 
                BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;  
            }
	}
        pthread_mutex_lock(&mutex);
        if(counter[count%2] == thread_count - 1)
        {
            swap(BMPSaveData,BMPData);
            counter[(count+1)%2]=0;
        }
        counter[count%2]++;
        pthread_mutex_unlock(&mutex);
        while(counter[count%2]<thread_count); 
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


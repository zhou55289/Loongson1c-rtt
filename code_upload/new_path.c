#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 24 
#define INF = 1000

int shortestpath();

typedef struct struct_graph {
    char vexs[MAXN];
    int vexnum;
    int edgnum;
    int matirx[MAXN][MAXN];
} Graph;

int pathmatirx[MAXN][MAXN];
int shortPath[MAXN][MAXN];

void short_path_floyd(Graph G, int P[MAXN][MAXN], int D[MAXN][MAXN]) {
    int v, w, k;
	char str1[50],str2[50],str3[50];
	
    for (v = 0; v < 24; v++) {
        for (w = 0; w < 24; w++) {
            D[v][w] = G.matirx[v][w];
            P[v][w] = w;
        }
    }



    for (k = 0; k < 24; k++) {

        for (v = 0; v < 24; v++) {

            for (w = 0; w < 24; w++) {
                if (D[v][w] > (D[v][k] + D[k][w])) {
                    D[v][w] = D[v][k] + D[k][w];
                    P[v][w] = P[v][k];
                }
            }
        }
    }

    v = 4;//现在的位置 
    w = 15;//出口 

   // printf("\n%d -> %d的最小路径为；%d\n", v + 1, w + 1, D[v][w]);
    //printf("The shortest path from %d to %d is %d\n", v + 1, w + 1, D[v][w]);
    sprintf(str1,"The shortest path from %d to %d is %d\n", v + 1, w + 1, D[v][w]);
    //printf("%s",str1);//打印最小路径的路程 
    k = P[v][w];
    

    //printf("path: %d", v + 1);
    sprintf(str2,"path: %d", v + 1);
    while (k != w) {

        //printf("-> %d", k + 1);
        sprintf(str3,"-> %d", k + 1);
        strcat(str2, str3);
        k = P[k][w];
    }
    //printf("-> %d", w + 1);
    sprintf(str3,"-> %d", w + 1);
    strcat(str2,str3);
    strcat(str1,str2);
    printf("%s",str1);//打印最小路径 
}

int shortestpath(){
	FILE* fp = NULL;
	Graph G;
	int p, q;

	fp = fopen("C:\\Users\\Salem\\Desktop\\information\\pathfinal.csv", "r");
	for (p = 0; p < 24; p++)
	{
		for (q = 0; q < 24; q++)
		{
			fscanf(fp, "%d", &G.matirx[p][q]);
			fseek(fp, 1L, SEEK_CUR);   /*fp指针从当前位置向后移动*/
		}
	}
	fclose(fp);
	// 读入路径矩阵 
	// 
	float T[24] = { 0 }, H[24] = { 0 }, C[24] = { 0 }, L[24] = { 0 };
	int x, y, m, n, i, j;
	float da[24];
	FILE* fe = NULL;
	fe = fopen("C:\\Users\\Salem\\Desktop\\information\\T.csv", "r");
	for (x = 0; x < 24; x++)
	{
		fscanf(fe, "%f", &T[x]);
		fseek(fe, 1L, SEEK_CUR);   /*fp指针从当前位置向后移动*/
	}
	fclose(fe);
	// 
	FILE* fc = NULL;
	fc = fopen("C:\\Users\\Salem\\Desktop\\information\\H.csv", "r");
	for (y = 0; y < 24; y++)
	{
		fscanf(fc, "%f", &H[y]);
		fseek(fc, 1L, SEEK_CUR);   /*fp指针从当前位置向后移动*/
	}
	fclose(fc);
	// 
	FILE* fx = NULL;
	fx = fopen("C:\\Users\\Salem\\Desktop\\information\\C.csv", "r");
	for (m = 0; m < 24; m++)
	{
		fscanf(fx, "%f", &C[m]);
		fseek(fx, 1L, SEEK_CUR);   /*fp指针从当前位置向后移动*/
	}
	fclose(fx);
	// 
	FILE* fa = NULL;
	fa = fopen("C:\\Users\\Salem\\Desktop\\information\\L.csv", "r");
	for (n = 0; n < 24; n++)
	{
		fscanf(fa, "%f", &L[n]);
		fseek(fa, 1L, SEEK_CUR);   /*fp指针从当前位置向后移动*/
	}
	fclose(fa);
	//读入信息矩阵（T温度，H湿度，C二氧化碳浓度，L光照强度）


	int R, z;
	for (R = 0; R < 24; R++)
	{
		if (T[R] >= 57 || H[R] <= 30 || C[R] >= 500 || L[R] <= 100)
		{
			for (z = 0; z < 24; z++)
			{
				G.matirx[R][z] = 1000;
				G.matirx[z][R] = 1000;
				G.matirx[R][R] = 0;
			}
		}
	}
	/*
	int a, b;
	for (a = 0; a < 24; a++)
	{
		for (b = 0; b < 24; b++)
		{
			printf("%d\t", G.matirx[a][b]);
		}
		printf("\n");
	}
	printf("T:");
	int flag;
	for (flag = 0; flag < 24; flag++)
	{
		printf("%f\t", T[flag]);
	}
	printf("\n\nH:");
	for (flag = 0; flag < 24; flag++)
	{
		printf("%f\t", H[flag]);
	}
	printf("\n\nC:");
	for (flag = 0; flag < 24; flag++)
	{
		printf("%f\t", C[flag]);
	}
	printf("\n\nL:");
	for (flag = 0; flag < 24; flag++)
	{
		printf("%f\t", L[flag]);
	}
	*/
	short_path_floyd(G, pathmatirx, shortPath); 
	return 0;

}

int main(){
	shortestpath();
}

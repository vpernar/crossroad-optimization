#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<semaphore.h>
#include<stdlib.h>
#include<sys/time.h>
#include <stdlib.h>

#define UBRZANJE 10


struct smer
{
	int indeks;
};

sem_t mutx;
sem_t mutx2;
sem_t KOLA[4];

int ACzeleno = 30;
int BDzeleno = 30;
int frek[]={22,8,14,12};
int kolaStanje[]={0,0,0,0};
int kolaOtisla[]={0,0,0,0};
int kolaPoCiklusu;
long long offset;
long long timePassed[] ={0,0,0,0};
int trenSEM = 0;
int WORK = 1;
double prosloSrednjeVreme = -1;
long long vreme()
{
	struct timeval now;
	gettimeofday(&now,NULL);
	return now.tv_sec*1000000LL+now.tv_usec;
}

void* producerKola(void* args)
{
	int indeks  = ((struct smer*)args)->indeks;
	long long start = vreme();
	long long getTime;
	while(WORK)
	{
		usleep(60.0/frek[indeks]*1000000/UBRZANJE); // 60/22=2.727272...

		getTime = vreme()-offset-start;
		printf("Kola u smeru %c su napravljena u %lld\n", indeks+'A', getTime);

		sem_wait(&mutx);
		sem_post(&KOLA[indeks]);

		kolaStanje[indeks]++;
		sem_post(&mutx);
	}
}


// AC su indeksi 0 i 2, parni
// BD su indeksi 1 i 3, neparni

int povecaj = 1, bolje = 1;
void* semafor()
{
	int i;
	double trenSrednjeVreme;
	long long start = vreme();
	double srednjeVremeIteraciji[4];
	while(WORK)
	{

		sem_wait(&mutx2);
		offset=vreme()-start;
		for(i = 0; i < 4; i++)
			kolaOtisla[i]=timePassed[i]=0;
		kolaPoCiklusu=0;
		sem_post(&mutx2);

		printf("ZELENO AC traje %d\n ", ACzeleno);
		trenSEM = 0;
		usleep(ACzeleno*1000000/UBRZANJE);


		sem_wait(&mutx2);
		offset=vreme()-start;
		sem_post(&mutx2);

		printf("ZELENO BD traje %d\n ", BDzeleno);
		trenSEM = 1;
		usleep(BDzeleno*1000000/UBRZANJE);

		trenSrednjeVreme=0;
		for(i=0;i<4;i++)
		{
			srednjeVremeIteraciji[i]=timePassed[i]*1.0/kolaOtisla[i];
			trenSrednjeVreme += srednjeVremeIteraciji[i]*(kolaOtisla[i]+kolaStanje[i]);
		}
		trenSrednjeVreme = trenSrednjeVreme / kolaPoCiklusu;
		if(prosloSrednjeVreme == -1)
		{
			prosloSrednjeVreme = trenSrednjeVreme;
		}
		else
		{
			if(trenSrednjeVreme > prosloSrednjeVreme)
			{
				if(bolje == 0 && povecaj)
				{
					povecaj = 0;
				}
				else if(bolje == 0 && povecaj == 0)
				{
					WORK = 0;
					printf("NAJBOLJE JE BILO : %lf\n", prosloSrednjeVreme);
				}


				bolje = 0;
			}
			else
			{
				bolje = 1;
			}

			if(povecaj)
			{
				ACzeleno -= 4;
				BDzeleno += 4;
			}
			else
			{
				ACzeleno += 4;
				BDzeleno -= 4;
			}
		}
		printf("SREDNJE VREME U ITERACIJI ____  ");
		for(int i = 0; i < 4; i++)
			printf("%lf ", srednjeVremeIteraciji[i]);
		printf("\n");

		printf("SREDNJE VREME U ITERACIJI ____  ");
		for(int i = 0; i < 4; i++)
			printf("%lf ", srednjeVremeIteraciji[i]*(kolaOtisla[i]+kolaStanje[i]));
		printf("\n");

		printf("SREDNJE VREME U ITERACIJI ____  ");
		for(int i = 0; i < 4; i++)
			printf("%d ", kolaOtisla[i]);
		printf("\n");

		printf("UKUPNO SREDNJE VREME U ITERACIJI ____ %lf\n", trenSrednjeVreme);

		printf("KRAJ ITERACIJE ____ ostalo je %d %d %d %d\n", kolaStanje[0], kolaStanje[1], kolaStanje[2], kolaStanje[3]);
	}
}


void* consumerKola(void* args)
{
	int indeks  = ((struct smer*)args)->indeks;
	long long start = vreme();
	long long getTime, startTime;

	while(WORK)
	{
		if((indeks&1) == trenSEM)
		{
			sem_wait(&KOLA[indeks]);


			usleep(2*1000000/UBRZANJE);

			sem_wait(&mutx);
			kolaStanje[indeks]--;

			sem_post(&mutx);

			getTime = vreme();

			sem_wait(&mutx2);
			kolaOtisla[indeks]++;
			kolaPoCiklusu++;
			timePassed[indeks]+=getTime - offset -start;
			sem_post(&mutx2);

			printf("Kola u smeru %c su OTISLA u %lld\n", indeks+'A', getTime- offset-start);
		}
	}
}


int main()
{
	int i;
	pthread_t producers[4];
	pthread_t consumers[4];
	pthread_t sem;

	sem_init(&mutx, 0, 1);
	sem_init(&mutx2, 0, 1);

	for(i = 0; i < 4; i++)
		sem_init(&KOLA[i], 0, 0);

	for(i = 0; i < 4; i++)
	{
		struct smer *arg = malloc(sizeof(struct smer));
		arg->indeks=i;

		pthread_create(&producers[i], NULL, producerKola, arg);
	}

	for(i = 0; i < 4; i++)
	{
		struct smer *arg = malloc(sizeof(struct smer));
		arg->indeks=i;

		pthread_create(&consumers[i], NULL, consumerKola, arg);
	}

	pthread_create(&sem, NULL, semafor, NULL);

	pthread_join(sem, NULL);
	return 0;
}

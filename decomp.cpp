#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>
#include <iostream>

// CONSTANTS
const float GRAIN_GROWS_PER_MONTH = 9.0;
const float ONE_DEER_EATS_PER_MONTH = 1.0;

const float AVG_PRECIP_PER_MONTH = 7.0; // average
const float AMP_PRECIP_PER_MONTH = 6.0; // plus or minus
const float RANDOM_PRECIP = 2.0;        // plus or minus noise

const float AVG_TEMP = 60.0;    // average
const float AMP_TEMP = 20.0;    // plus or minus
const float RANDOM_TEMP = 10.0; // plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0; 

// PROTOTYPES
float SQR(float x);
float Ranf(unsigned int *seedp, float low, float high);
int Ranf(unsigned int *seedp, int ilow, int ihigh);
void InitBarrier(int);
void WaitBarrier();
void Deer();
void Grain();
void Watcher();
void DeathStar();

// GLOBALS
int NowYear;  // 2021 to 2026
int NowMonth; // 0 - 11

float NowPrecip; // inches of rain per month
float NowTemp;   // temperature this month
float NowHeight; // grain height in inches
int NowNumDeer;  // number of deer in the current population

unsigned int seed = 0;
float x = Ranf(&seed, -1.f, 1.f);

omp_lock_t Lock;
int NumInThreadTeam;
int NumAtBarrier;
int NumGone;

int RebelsFound; // tracks whether the death start obliterated the Rebel farming operation 0 = no, 1 = yes

int main(int argc, char *argv[])
{
#ifndef _OPENMP
    fprintf(stderr, "No OpenMP Support!\n");
    return 1;
#endif

    NowNumDeer = 1;
    NowHeight = 1.;
    RebelsFound = 0;

    omp_set_num_threads(4);
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            Deer();
        }
        
        #pragma omp section
        {
            Grain();
        }

        #pragma omp section
        {
            DeathStar();
        }

        #pragma omp section
        {
            Watcher();
        }
    }
}

float SQR(float x)
{
    return x * x;
}

float Ranf(unsigned int *seedp, float low, float high)
{
    float r = (float) rand_r(seedp);
    return( low + r * (high - low) / (float)RAND_MAX);
}

int Ranf(unsigned int *seedp, int ilow, int ihigh)
{
    float low = (float)ilow;
    float high = (float)ihigh + 0.9999f;
    return (int)(Ranf(seedp, low, high));
}

void InitBarrier(int n)
{
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock(&Lock);
}

void WaitBarrier()
{
    omp_set_lock(&Lock);
    {
        NumAtBarrier++;
        if (NumAtBarrier == NumInThreadTeam)
        {
            NumGone = 0;
            NumAtBarrier = 0;
            while(NumGone != NumInThreadTeam - 1)
                omp_unset_lock(&Lock);
            return;
        }
    }
    omp_unset_lock(&Lock);
    while(NumAtBarrier != 0);
    #pragma omp atomic
    NumGone++;
}

void Deer()
{
    while (NowYear < 2027)
    {
        // while (NowMonth < 12)
        // {
            int nextNumDeer = NowNumDeer;
            int carryingCapacity = (int)(NowHeight);
            
            if (nextNumDeer < carryingCapacity)
            {
                nextNumDeer++;
            }
            else
            {
                if (nextNumDeer > carryingCapacity)    
                {
                    nextNumDeer--;
                }
            }
            if (nextNumDeer < 0)
            {
                nextNumDeer = 0;
            }
            #pragma omp barrier

            NowNumDeer = nextNumDeer;

            #pragma omp barrier
        // }
    }
}

void Grain()
{
    while (NowYear < 2027)
    {
        // while (NowMonth < 12)
        // {
            float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
            float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));

            float nextHeight = NowHeight;
            nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
            nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;

            if (nextHeight < 0.)
                nextHeight = 0.;
            
            #pragma omp barrier

            NowHeight = nextHeight;

            #pragma omp barrier
        // }
    }
}

// The Rebel Alliance is feeding their guerilla armies with grain and venison - will the Death Star 
// find their farming operation? If the Death Star finds the Rebel breadbox, it will destroy all 
// their efforts, forcing them to start anew on another planet.
void DeathStar()
{
    while (NowYear < 2027)
    {
        int deathStar;
        deathStar = rand() % 3; // generate a random number between 0 and 3
        RebelsFound = 0; // reset to False
        #pragma omp barrier

        // Rebels discovered! 
        if (deathStar == 1)
        {
            RebelsFound = 1;
        }
        // Rebels are safe for another month!
        else
        {
            // do nothing!
            RebelsFound = 0;
        }
        #pragma omp barrier
    }
}

void Watcher()
{
    NowYear = 2021;
    while (NowYear < 2027)
    {
        NowMonth = 0;
        while (NowMonth < 12)
        {
            // calculate new weather data
            float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);

            float temp = AVG_TEMP - AMP_TEMP * cos(ang);
            NowTemp = temp + Ranf(&seed, -RANDOM_TEMP, RANDOM_TEMP);

            float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
            NowPrecip = precip + Ranf(&seed, -RANDOM_PRECIP, RANDOM_PRECIP);
            if (NowPrecip < 0.)
                NowPrecip = 0.;

            #pragma omp barrier
            
            // check for Death Star
            if (RebelsFound == 1)
            {
                NowHeight = 0.;
                NowNumDeer = 0;
            }

            float NowHeightC = NowHeight * 2.54;
            float NowTempC = (5./9.) * (NowTemp - 32);
            float NowPrecipC = NowPrecip * 2.54;

            // printf("%d/%d: Grain Height: %6.2f, Deer: %d, Temp: %6.2f, Precip: %6.2f\n", NowMonth, NowYear, NowHeight, NowNumDeer, NowTemp, NowPrecip);
            printf("%d/%d,%6.2f,%d,%6.2f,%6.2f,%d\n", NowMonth, NowYear, NowHeightC, NowNumDeer, NowTempC, NowPrecipC, RebelsFound);

            #pragma omp barrier

            NowMonth++;
        }
        NowYear++;
    }
}
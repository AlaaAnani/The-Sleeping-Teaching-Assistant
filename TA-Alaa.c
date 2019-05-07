#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     //For sleep calls
#include <time.h>       //For random time wait
#include <semaphore.h>  //Using semaphores as signals for availability of business 
#define MAX_STUDENT_COUNT 64
#define MAX_NUMBER_OF_SEATS 3
#define MAX_SLEEP_TIME 7

pthread_t TA, *Students; //TA-> A single thread. *Students-> Pointer to an array of threads (mimicking processes) 

void *StudentBehavior(void *threadID);  //Passing thread ID to assign numbers to students
void *TABehavior();
int StudentsNumber = 5;

//Global signals (Semaphores)
pthread_mutex_t VolatilityMutex;


sem_t SemTAAsleep;
sem_t SemStudent;

volatile int QHead = 0, StudentInside = 0;
volatile int Seats[MAX_NUMBER_OF_SEATS]; //To trace thread ID(student ID) ocuupying each seat
volatile int QTail = 0, SlotsOccupied = 0;

int main(int argc, char* argv[])
{
    

    //Get number of students
    if (argc < 2)
	{
		printf("Number of Students not specified. Using default (5) students.\n");
	}
	else
	{
        StudentsNumber = atoi(argv[1]);
		printf("Number of Students specified. Creating %d threads.\n", StudentsNumber);		
	}

    //////Beginning of Initialization/////
    srand(time(NULL));
    //Initialization of sempahores/signals
    sem_init(&SemTAAsleep, 0, 0);   //0-> Semaphore shared among threads not processes
                                    //0-> Initial sem value (TA is initially asleep)
    sem_init(&SemStudent,  0, 0);

    //Mutex over chair access and volatile vars
    pthread_mutex_init(&VolatilityMutex, NULL);
    //////End of Initialization/////

    //Allocate mem for students thread array then create it
    Students = (pthread_t*)malloc(sizeof(pthread_t)*StudentsNumber);
    for(int i=0;i< StudentsNumber; i++)
        pthread_create(&Students[i], NULL, StudentBehavior, (void*)i);
    pthread_create(&TA, NULL, TABehavior, NULL);
    pthread_join(TA, NULL);
    for(int i=0;i< StudentsNumber; i++)
        pthread_join(Students[i], NULL);

    free(Students);
    return 0;
} 

void *StudentBehavior(void *threadID)
{
    int WaitRandTime;
    long SID = (long)threadID;
    int localSlotCount = 0;
    while(1)
    {
        printf("Student %ld is working on the assignment alone. \n", SID);
        WaitRandTime = rand() % 10 + 1;
        sleep(WaitRandTime);                //Wait for a random time
        printf("Student %ld needs the TA. \n", SID);

        pthread_mutex_lock(&VolatilityMutex);
            localSlotCount = SlotsOccupied;
        pthread_mutex_unlock(&VolatilityMutex);

        if(localSlotCount >= MAX_NUMBER_OF_SEATS)
            printf("Student %ld will return at another time. \n", SID);
        else
        {
            // < 3
            //0
            //If first student, signal waking up the TA
            pthread_mutex_lock(&VolatilityMutex);
                Seats[QTail] = SID;
                SlotsOccupied++;
                printf("Student %ld sat on chair %d. Chairs remaining = %d. \n", SID, QTail, MAX_NUMBER_OF_SEATS-SlotsOccupied);
                QTail = (QTail + 1) % MAX_NUMBER_OF_SEATS; 
                localSlotCount=SlotsOccupied;    
            pthread_mutex_unlock(&VolatilityMutex);

            if(localSlotCount == 1 && StudentInside == 0)
            {
                sem_post(&SemTAAsleep); //Wake TA up
            }
            else
            {
                printf("Student %ld is waiting for TA.\n", SID);
            }    



                //printf("Student %ld is enqueued. \n", SID);
                sem_wait(&SemStudent);
			    printf("Student %ld is done with TA and left. \n", SID);
		
         
            
        }
        
    }
}

void *TABehavior()
{
    int localSlotsCount = 0;
    int localSID = 0;
    while(1)
    {
        //Wait till the TA wakes up (TA is woken up typically by any first student arriving))
        sem_wait(&SemTAAsleep);
        printf("TA is awakened. \n");
        
            //If TA is awaken, lock chairs
            pthread_mutex_lock(&VolatilityMutex);
            StudentInside = 1;
            localSlotsCount = SlotsOccupied;
            pthread_mutex_unlock(&VolatilityMutex);
            if(localSlotsCount == 0)
            {
                printf("False alarm. \n");  //To do: Reset TA awake mutex and slots count
                continue;
            }

            while(localSlotsCount != 0)
            {
                pthread_mutex_lock(&VolatilityMutex);
                    printf("TA has %d students to serve. \n", SlotsOccupied);
                    printf("TA is serving student with SID %d who was on seat %d.\n", Seats[QHead], QHead);
                    SlotsOccupied--;
                    localSID = Seats[QHead];
                    QHead=(QHead+1)%MAX_NUMBER_OF_SEATS;
                pthread_mutex_unlock(&VolatilityMutex);
                sleep(rand()%4 +1 );
                printf("TA is done helping SID = %d.\n", localSID);
                sem_post(&SemStudent);  //Student is served

                pthread_mutex_lock(&VolatilityMutex);
                    localSlotsCount = SlotsOccupied;
                pthread_mutex_unlock(&VolatilityMutex);
                
            }
            sem_init(&SemTAAsleep, 0,0);

            pthread_mutex_lock(&VolatilityMutex);
                StudentInside = 0;
            pthread_mutex_unlock(&VolatilityMutex);

        
    }
}


